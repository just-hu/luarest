#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <uv.h>
#include <http_parser.h>

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

#define RESPONSE \
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/plain\r\n" \
  "Content-Length: 12\r\n" \
  "\r\n" \
  "hello world\n"

static uv_loop_t* uv_loop;
static uv_tcp_t server;
static http_parser_settings parser_settings;

static uv_buf_t resbuf;

typedef struct {
  uv_tcp_t handle;
  http_parser parser;
  uv_write_t write_req;
  int request_num;
} client_t;

void on_close(uv_handle_t* handle) {
  client_t* client = (client_t*) handle->data;

  LOGF("[ %5d ] connection closed", client->request_num);

  free(client);
}

uv_buf_t on_alloc(uv_handle_t* client, size_t suggested_size) {
  uv_buf_t buf;
  buf.base = (char*)malloc(suggested_size);
  buf.len = suggested_size;
  return buf;
}

void on_read(uv_stream_t* tcp, ssize_t nread, uv_buf_t buf) {
  ssize_t parsed;

  client_t* client = (client_t*) tcp->data;

  if (nread >= 0) {
    parsed = http_parser_execute(
        &client->parser, &parser_settings, buf.base, nread);
    if (parsed < nread) {
      LOG_ERROR("parse error");
      uv_close((uv_handle_t*) &client->handle, on_close);
    }
  } else {
    uv_err_t err = uv_last_error(uv_loop);
    if (err.code != UV_EOF) {
      UVERR(err, "read");
    }
  }

  free(buf.base);
}

static int request_num = 1;

void on_connect(uv_stream_t* server_handle, int status) {
	int r;
	client_t* client;

	CHECK(status, "connect");

  assert((uv_tcp_t*)server_handle == &server);

  client = (client_t*)malloc(sizeof(client_t));
  client->request_num = request_num;

  LOGF("[ %5d ] new connection", request_num++);

  uv_tcp_init(uv_loop, &client->handle);
  http_parser_init(&client->parser, HTTP_REQUEST);

  client->parser.data = client;
  client->handle.data = client;

  r = uv_accept(server_handle, (uv_stream_t*)&client->handle);
  CHECK(r, "accept");

  uv_read_start((uv_stream_t*)&client->handle, on_alloc, on_read);
}

void after_write(uv_write_t* req, int status) {
  CHECK(status, "write");

  uv_close((uv_handle_t*)req->handle, on_close);
}

int on_headers_complete(http_parser* parser) {
  client_t* client = (client_t*) parser->data;
  
  LOGF("[ %5d ] http message parsed", client->request_num);

  uv_write(
      &client->write_req,
      (uv_stream_t*)&client->handle,
      &resbuf,
      1,
      after_write);

  return 1;
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
	application* apps = NULL;
	
	if (argc < 2) {
		usage();
		return(1);
	}

	lret = create_applications(apps, argv[1]);

	if (lret != LUAREST_SUCCESS || apps == NULL) {
		printf("Error: No applications could be loaded can't start!\n");
		return(1);
	}

	parser_settings.on_headers_complete = on_headers_complete;
	uv_loop = uv_default_loop();
	
	resbuf.base = RESPONSE;
	resbuf.len = sizeof(RESPONSE);
	
	ret = uv_tcp_init(uv_loop, &server);
	CHECK(ret, "init");
	
	address = uv_ip4_addr("0.0.0.0", 8000);
	
	ret = uv_tcp_bind(&server, address);
	CHECK(ret, "bind");
	
	uv_listen((uv_stream_t*)&server, 128, on_connect);
	
	LOG("luarest is listening on port 8000");
	
	uv_run(uv_loop);

	free_applications(apps);

	return(0);
}