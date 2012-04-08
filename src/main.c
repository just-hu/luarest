#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uv.h>
#include <uv-private/ngx-queue.h>
#include <http_parser.h>

#include "thirdparty/utlist.h"

#include "app.h"

#define CHECK(r, msg) \
  if (r) { \
    uv_err_t err = uv_last_error(uv_loop); \
    fprintf(stderr, "%s: %s\n", msg, uv_strerror(err)); \
    exit(1); \
  }
#define UVERR(err, msg) fprintf(stderr, "%s: %s\n", msg, uv_strerror(err))
#define LOG(msg) puts(msg);
#define LOGF(fmt, params) printf(fmt "\n", params);
#define LOG_ERROR(msg) puts(msg);

/* Set keep-alive timeout to 75s */
#define HTTP_KEEP_ALIVE_TIMEOUT_SEC 75

#define RESPONSE_HEADER "HTTP/1.1 200 OK\r\n"
#define RESPONSE_CONTENT_TYPE "Content-Type: %s\r\n"
#define RESPONSE_CONTENT_LENGTH "Content-Length: %d\r\n"
#define RESPONSE_CONNECTION_KEEP_ALIVE "Connection: Keep-Alive\r\n"
#define RESPONSE_HEADER_COMPLETE "\r\n"

static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;
static application* apps = NULL;

typedef struct header_t {
	UT_string* field;
	UT_string* value;
} header_t;

typedef struct response_t {
	ngx_queue_t queue;

} response_t;

typedef struct client_t {
  uv_tcp_t handle;
  http_parser* parser;
  uv_write_t write_req;
  int conn_num;
  UT_string* url;
  UT_string* path;
  UT_string* query;
  luarest_method req_method;
  int keep_alive_header;
  int should_keep_alive;
  int message_complete;
  int num_header_fields;
  int num_header_values;
  int idle_time_sec;
  header_t* headers;
  int headers_len;
  struct client_t* prev;
  struct client_t* next;
} client_t;

static int conn_counter;
static client_t* connections = NULL;

/**
 * 
 *
 */
static void on_close(uv_handle_t* handle) {
	client_t* client = (client_t*)handle->data;
	
	LOGF("[ %5d ] connection closed", client->conn_num);
	
	DL_DELETE(connections, client);

	free(client);
}
/**
 * This timer goes off every 5s and increments the idle-time on all 
 * connections, once the specified KEEP-ALIVE timeout is reached 
 * the connection is closed by the server
 *
 */
static void on_timeout_timer(uv_timer_t* timer, int status)
{
	client_t* client;
	CHECK(status, "timeout timer");
	DL_FOREACH(connections, client) {
		client->idle_time_sec += 5;
		if (client->idle_time_sec >= HTTP_KEEP_ALIVE_TIMEOUT_SEC) {
			printf("Keep-Alive timeout on connection %d, timeout %d\n", client->conn_num, client->idle_time_sec);
			uv_close((uv_handle_t*) &client->handle, on_close);
		}
	}
	uv_timer_start(timer, on_timeout_timer, 5000, 0);
}
/**
 * 
 *
 */
static uv_buf_t on_alloc(uv_handle_t* client, size_t suggested_size) {
	uv_buf_t buf;
	buf.base = (char*)malloc(suggested_size);
	buf.len = suggested_size;
	return(buf);
}
/**
 * 
 *
 */
static void on_write(uv_write_t* req, int status) {
	CHECK(status, "write");
}
/**
 * 
 *
 */
static void process_request(client_t* client)
{ 
	luarest_status res = LUAREST_SUCCESS;
	uv_buf_t buf;
	int i;

	UT_string* sbuf;
	UT_string* resp;
	luarest_response res_code;
	luarest_content_type content_type;

	utstring_new(resp);
	
	res = invoke_application(apps, client->path, client->req_method, &res_code, &content_type, resp);

	utstring_new(sbuf);
	utstring_printf(sbuf, RESPONSE_HEADER);
	utstring_printf(sbuf, RESPONSE_CONTENT_TYPE, luarest_content_type_str[content_type]);
	utstring_printf(sbuf, RESPONSE_CONTENT_LENGTH, utstring_len(resp));
	if (client->keep_alive_header) {
		/* If its HTTP/1.0 and the Connection: Keep-Alive header is present we have to
		   respond with the same header and make sure not to close the connection */
		utstring_printf(sbuf, RESPONSE_CONNECTION_KEEP_ALIVE);
	}
	utstring_printf(sbuf, RESPONSE_HEADER_COMPLETE);
	utstring_concat(sbuf, resp);

	utstring_free(resp);
	
	buf.base = utstring_body(sbuf);
	buf.len = utstring_len(sbuf);

	uv_write(&client->write_req, (uv_stream_t*)&client->handle, &buf, 1, on_write);
	client->idle_time_sec = 0;

	utstring_free(sbuf);
	
	/* reset for next request */
	utstring_free(client->url);
	utstring_free(client->path);
	utstring_free(client->query);
	client->num_header_fields = 0;
	client->num_header_values = 0;
	for (i = 0; i < client->num_header_fields; i++) {
		utstring_free(client->headers[i].field);
		utstring_free(client->headers[i].value);
	}
	free(client->headers);

	if (!client->should_keep_alive) {
		uv_close((uv_handle_t*) &client->handle, on_close);
	}
}
/**
 * This is called until every thing is read from the socket
 *
 */
static void on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf) {
	ssize_t parsed;
	client_t* client = (client_t*) tcp->data;

	if (nread < 0) {
		if (buf.base) {
			free(buf.base);
		}
		uv_close((uv_handle_t*)tcp, on_close);
		return;
	}

	client->idle_time_sec = 0;

	if (client->parser == NULL) {
		client->parser = (http_parser*)malloc(sizeof(http_parser));
		http_parser_init(client->parser, HTTP_REQUEST);
		client->parser->data = client;
	}
	
	parsed = http_parser_execute(client->parser, &parser_settings, buf.base, nread);
	if (parsed < nread) {
		LOG_ERROR("parse error");
		uv_close((uv_handle_t*) &client->handle, on_close);
	}
	
	free(buf.base);

	if (client->message_complete) {
		free(client->parser);
		client->parser = NULL;
		process_request(client);
	}
}
/**
 * 
 *
 */
static void on_connect(uv_stream_t* server_handle, int status) {
	int r;
	client_t* client;

	CHECK(status, "connect");

	assert((uv_tcp_t*)server_handle == &server);
	
	client = (client_t*)malloc(sizeof(client_t));
	client->parser = NULL;
	client->url = NULL;
	client->num_header_fields = 0;
	client->num_header_values = 0;
	client->conn_num = ++conn_counter;
	client->keep_alive_header = 0;
	client->message_complete = 0;
	client->should_keep_alive = 1;
	client->idle_time_sec = 0;
	client->headers = NULL;

	LOGF("[ %5d ] new connection", client->conn_num);
	
	uv_tcp_init(uv_loop, &client->handle);
	client->handle.data = client;
	
	r = uv_accept(server_handle, (uv_stream_t*)&client->handle);
	CHECK(r, "accept");

	DL_APPEND(connections, client);
	
	uv_read_start((uv_stream_t*)&client->handle, on_alloc, on_read);
}
/**
 *
 *
 */
static luarest_status map_http_method(luarest_method* rm, char m)
{
	switch (m) {
		case 1:
			*rm = HTTP_METHOD_GET;
			break;
		case 3:
			*rm = HTTP_METHOD_POST;
			break;
		case 4:
			*rm = HTTP_METHOD_PUT;
			break;
		case 0:
			*rm = HTTP_METHOD_DELETE;
			break;
		case 6:
			*rm = HTTP_METHOD_OPTION;
			break;
		case 2:
			*rm = HTTP_METHOD_HEAD;
			break;
		default:
			return(LUAREST_ERROR);
	}
	return(LUAREST_SUCCESS);
}
/**
 *
 *
 */
static int on_headers_complete(http_parser* parser) {
	luarest_method m;
	luarest_status res = LUAREST_SUCCESS;
	client_t* client = (client_t*)parser->data;
	struct http_parser_url* hpu = (struct http_parser_url*)malloc(sizeof(struct http_parser_url));

	res = map_http_method(&m, parser->method);
	client->req_method = m;

	if (parser->flags & F_CONNECTION_KEEP_ALIVE) {
		client->keep_alive_header = 1;
	}

	http_parser_parse_url(utstring_body(client->url), utstring_len(client->url), 0, hpu);
	if (hpu->field_set & (1 << (UF_PATH))) {
		utstring_bincpy(client->path, utstring_body(client->url)+hpu->field_data[UF_PATH].off, hpu->field_data[UF_PATH].len);
	}
	if (hpu->field_set & (1 << (UF_QUERY))) {
		utstring_bincpy(client->query, utstring_body(client->url)+hpu->field_data[UF_QUERY].off, hpu->field_data[UF_QUERY].len);
	}
	free(hpu);

	return(0);
}
/**
 *
 *
 */
static int on_url(http_parser* parser, const char *at, size_t length)
{
	client_t* client = (client_t*)parser->data;
	
	utstring_bincpy(client->url, at, length);
	
	return(0);
}
/**
 *
 *
 */
static int on_header_field(http_parser* parser, const char* at, size_t lenght)
{
	client_t* client = (client_t*)parser->data;

	if (client->num_header_fields == client->num_header_values) {
		if (client->num_header_fields == 0) {
			client->headers = (header_t*)malloc(sizeof(header_t)*2);
			client->headers_len = 2;
		}
		else if (client->num_header_fields == client->headers_len) {
			client->headers_len *= 2;
			client->headers = (header_t*)realloc(client->headers, sizeof(header_t)*client->headers_len);
		}
		client->num_header_fields++;
		utstring_new(client->headers[client->num_header_fields-1].field);
	}

	utstring_bincpy(client->headers[client->num_header_fields-1].field, at, lenght);

	return(0);
}
/**
 *
 *
 */
static int on_header_value(http_parser* parser, const char* at, size_t lenght)
{
	client_t* client = (client_t*)parser->data;

	if (client->num_header_fields != client->num_header_values) {
		client->num_header_values++;
		utstring_new(client->headers[client->num_header_values-1].value);
	}

	utstring_bincpy(client->headers[client->num_header_values-1].value, at, lenght);

	return(0);
}
/**
 *
 *
 */
static int on_message_begin(http_parser* parser)
{
	client_t* client = (client_t*)parser->data;

	utstring_new(client->url);
	utstring_new(client->path);
	utstring_new(client->query);
	client->message_complete = 0;

	return(0);
}
/**
 *
 *
 */
static int on_message_complete(http_parser* parser)
{
	client_t* client = (client_t*)parser->data;

	client->should_keep_alive = http_should_keep_alive(client->parser);

	client->message_complete = 1;

	return(0);
}
/**
 *
 *
 */
static void usage()
{
	printf("Usage: luarest <app-dir>\n");
}
/**
 *
 *
 */
int main(int argc, char *argv[]) {
	int ret;
	luarest_status lret;
	struct sockaddr_in address;
	uv_timer_t timeout_timer;
	
	if (argc < 2) {
		usage();
		return(1);
	}

	lret = create_applications(&apps, argv[1]);

	if (lret != LUAREST_SUCCESS || apps == NULL) {
		printf("Error: No applications could be loaded can't start!\n");
		return(1);
	}

	parser_settings.on_url = on_url;
	parser_settings.on_header_field = on_header_field;
	parser_settings.on_header_value = on_header_value;
	parser_settings.on_headers_complete = on_headers_complete;
	parser_settings.on_message_begin = on_message_begin;
	parser_settings.on_message_complete = on_message_complete;
	uv_loop = uv_default_loop();
	
	ret = uv_tcp_init(uv_loop, &server);
	CHECK(ret, "init");
	
	address = uv_ip4_addr("0.0.0.0", 8000);
	
	ret = uv_tcp_bind(&server, address);
	CHECK(ret, "bind");
	
	uv_listen((uv_stream_t*)&server, 128, on_connect);
	
	LOG("luarest is listening on port 8000");

	/* setup time-out timer */
	uv_timer_init(uv_loop, &timeout_timer);
	uv_timer_start(&timeout_timer, on_timeout_timer, 5000, 0);
	
	uv_run(uv_loop);

	uv_timer_stop(&timeout_timer);
	free_applications(apps);

	return(0);
}