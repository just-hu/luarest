#ifndef __LUAREST_APP_H__
#define __LUAREST_APP_H__

#include "luarest.h"
#include "thirdparty/uthash.h"
#include "thirdparty/utstring.h"

#include <lua.h>

/*-----------------------------------------------------------------------------
 * Data structures
 *----------------------------------------------------------------------------*/
typedef enum luarest_method {
	HTTP_METHOD_GET = 1,
	HTTP_METHOD_POST = 2,
	HTTP_METHOD_PUT = 3,
	HTTP_METHOD_DELETE = 4,
	HTTP_METHOD_OPTION = 5,
	HTTP_METHOD_HEAD = 6,
} luarest_method;

typedef enum luarest_response {
	HTTP_RESPONSE_OK = 1,
	HTTP_RESPONSE_CREATED = 2,
	HTTP_RESPONSE_NO_CONTENT = 3,
	HTTP_RESPONSE_NOT_ACCEPTABLE = 4,
	HTTP_RESPONSE_NOT_MODIFIED = 5,
	HTTP_RESPONSE_SEE_OTHER = 6,
	HTTP_RESPONSE_SERVER_ERROR = 7,
	HTTP_RESPONSE_TEMPORARY_REDIRECT = 8
} luarest_response;

typedef enum luarest_content_type {
	CONTENT_TYPE_PLAIN = 1,
	CONTENT_TYPE_HTML = 2,
	CONTENT_TYPE_JSON = 3
} luarest_content_type;

typedef struct service {
	UT_string* key;
	luarest_method method;
	UT_string* path;
	int callback_ref;
	UT_hash_handle hh;
} service;

typedef struct application {
	UT_string* name;
	lua_State* lua_state;
	service* s;
	UT_hash_handle hh;
} application;

/*-----------------------------------------------------------------------------
 * Functions prototypes
 *----------------------------------------------------------------------------*/
luarest_status create_applications(application** apps, char* app_dir);
luarest_status free_applications(application* apps);
luarest_status invoke_application(application* apps, UT_string* url, luarest_method m, luarest_response* res_code, 
	luarest_content_type* con_type, UT_string* res_buf);

/*-----------------------------------------------------------------------------
 * Globals
 *----------------------------------------------------------------------------*/
static const char luarest_content_type_str[][18] = {
	"", /* Sentinel */
	"text/plain",
	"text/html",
	"application/json"
};

#endif