#ifndef __LUAREST_APP_H__
#define __LUAREST_APP_H__

#include "luarest.h"
#include "thirdparty/uthash.h"
#include "thirdparty/utstring.h"

#include <lua.h>

/*-----------------------------------------------------------------------------
 * Data structures
 *----------------------------------------------------------------------------*/
typedef struct application {
	UT_string* name;
	UT_string* url_mapping;
	lua_State* lua_state;
	UT_hash_handle hh;
} application;

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

/*-----------------------------------------------------------------------------
 * Functions prototypes
 *----------------------------------------------------------------------------*/
luarest_status create_applications(application* apps, char* app_dir);
luarest_status free_applications(application* apps);

#endif