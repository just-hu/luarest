#define APP_ENTRY_POINT TEXT("main.lua")

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "app.h"

#define LUA_ENUM(L, name, val) \
  lua_pushlstring(L, #name, sizeof(#name)-1); \
  lua_pushnumber(L, val); \
  lua_settable(L, -3);

#define LUA_USERDATA_APPLICATION "luarest.application"

/* forward decls */
static int l_register(lua_State* state);

static const struct luaL_Reg l_application [] = {
	{"register", l_register},
	{NULL, NULL} /* sentinel */
};

/**
 * DEBUG function
 *
 */
static void stackDump(lua_State *L)
{
    int i;
    int top = lua_gettop(L);
    for (i = 1; i <= top; i++) {
        int t = lua_type(L, i);
        switch(t) {
            case LUA_TSTRING: {
                printf("'%s'", lua_tostring(L, i));
                break;
            }
            case LUA_TBOOLEAN: {
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;
            }
            case LUA_TNUMBER: {
                printf("%g", lua_tonumber(L, t));
                break;
            }
            default: {
                printf("%s", lua_typename(L, t));
                break;
            }
        }
        printf("  ");
    }
    printf("\n");
}
/**
 *
 *
 */
static luarest_status map_response(luarest_response* lr, int res)
{
	switch (res) {
		case 1:
			*lr = HTTP_RESPONSE_OK;
			break;
		case 2:
			*lr = HTTP_RESPONSE_CREATED;
			break;
		case 3:
			*lr = HTTP_RESPONSE_NO_CONTENT;
			break;
		case 4:
			*lr = HTTP_RESPONSE_NOT_ACCEPTABLE;
			break;
		case 5:
			*lr = HTTP_RESPONSE_NOT_MODIFIED;
			break;
		case 6:
			*lr = HTTP_RESPONSE_SEE_OTHER;
			break;
		case 7:
			*lr = HTTP_RESPONSE_SERVER_ERROR;
			break;
		case 8:
			*lr = HTTP_RESPONSE_TEMPORARY_REDIRECT;
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
static luarest_status map_contype(luarest_content_type *ct, int type)
{
	switch (type) {
		case 1:
			*ct = CONTENT_TYPE_PLAIN;
			break;
		case 2:
			*ct = CONTENT_TYPE_HTML;
			break;
		case 3:
			*ct = CONTENT_TYPE_JSON;
			break;
		default:
			return(LUAREST_ERROR);
	}
	return(LUAREST_SUCCESS);
}
/**
 * LUA syntax: application.register(method, url, callback)
 *
 * Return: boolean true on success
 *
 */
static int l_register(lua_State* state)
{
	service* s;
	application* a = (application*)luaL_checkudata(state, 1, LUA_USERDATA_APPLICATION);
	int method = luaL_checkint(state, 2);
	const char* url = luaL_checkstring(state, 3);
	int ref = luaL_ref(state, LUA_REGISTRYINDEX);

	s = (service*)malloc(sizeof(service));
	utstring_new(s->key);
	utstring_printf(s->key, "M%d#P%s", method, url);
	switch (method)
	{
		case 1:
			s->method = HTTP_METHOD_GET;
			break;
		case 2:
			s->method = HTTP_METHOD_POST;
			break;
		case 3:
			s->method = HTTP_METHOD_PUT;
			break;
		case 4:
			s->method = HTTP_METHOD_DELETE;
			break;
		case 5:
			s->method = HTTP_METHOD_OPTION;
			break;
		case 6:
			s->method = HTTP_METHOD_HEAD;
			break;
	}
	utstring_new(s->path);
	utstring_printf(s->path, url);
	s->callback_ref = ref;
	HASH_ADD_KEYPTR(hh, a->s, utstring_body(s->key), utstring_len(s->key), s);

	return(1);
}
/**
 *
 *
 */
static int luaopen_luarestlibs(lua_State *state)
{
	lua_newtable(state);
	lua_setglobal(state, "luarest");
	lua_getglobal(state, "luarest");
	{
		int i = 1;
		
		/* register HTTP_METHODS */
		LUA_ENUM(state, HTTP_METHOD_GET, i++);
		LUA_ENUM(state, HTTP_METHOD_POST, i++);
		LUA_ENUM(state, HTTP_METHOD_PUT, i++);
		LUA_ENUM(state, HTTP_METHOD_DELETE, i++);
		LUA_ENUM(state, HTTP_METHOD_OPTION, i++);
		LUA_ENUM(state, HTTP_METHOD_HEAD, i++);

		/* register HTTP_RESPONSE */
		i = 1;
		LUA_ENUM(state, HTTP_RESPONSE_OK, i++);
		LUA_ENUM(state, HTTP_RESPONSE_CREATED, i++);
		LUA_ENUM(state, HTTP_RESPONSE_NO_CONTENT, i++);
		LUA_ENUM(state, HTTP_RESPONSE_NOT_ACCEPTABLE, i++);
		LUA_ENUM(state, HTTP_RESPONSE_NOT_MODIFIED, i++);
		LUA_ENUM(state, HTTP_RESPONSE_SEE_OTHER, i++);
		LUA_ENUM(state, HTTP_RESPONSE_SERVER_ERROR, i++);
		LUA_ENUM(state, HTTP_RESPONSE_TEMPORARY_REDIRECT, i++);

		i = 1;
		LUA_ENUM(state, CONTENT_TYPE_PLAIN, i++);
		LUA_ENUM(state, CONTENT_TYPE_HTML, i++);
		LUA_ENUM(state, CONTENT_TYPE_JSON, i++);
	}
	
	luaL_newmetatable(state, LUA_USERDATA_APPLICATION);
	lua_pushvalue(state, -1);
	lua_setfield(state, -2, "__index");
	luaL_register(state, NULL, l_application);

	return(1);
}
/**
 *
 *
 */
static luarest_status invoke_lua(lua_State* state, int ref_cb, luarest_response* res_code, 
	luarest_content_type* con_type, UT_string* res_buf)
{
	lua_rawgeti(state, LUA_REGISTRYINDEX, ref_cb);
	lua_pushnil(state);
	lua_pushnil(state);
	lua_pushnil(state);
	if (lua_pcall(state, 3, 3, 0) != 0) {
		printf("Error calling service-callback: %s\n!", lua_tostring(state, -1));
		return(LUAREST_ERROR);
	}
	map_response(res_code, luaL_checkint(state, -3));
	map_contype(con_type, luaL_checkint(state, -2));
	utstring_printf(res_buf, luaL_checkstring(state, -1));
	lua_pop(state, 3);
	return(LUAREST_SUCCESS);
}
/**
 *
 *
 */
static luarest_status verify_application(application** apps, const char* appName, UT_string* path)
{
	int ret;
	lua_State* ls = luaL_newstate();
	application* app;
	
	luaL_openlibs(ls);
	luaopen_luarestlibs(ls);
        
    ret = luaL_loadfile(ls, utstring_body(path));
    if (ret != 0) {
		printf("Couldn't load file: %s\n", lua_tostring(ls, -1));
		lua_close(ls);
		return(LUAREST_ERROR);
    }
	ret = lua_pcall(ls, 0, 0, 0);
    if (ret != 0) {
        printf("Couldn't execute LUA Script %s\n", lua_tostring(ls, -1));
		lua_close(ls);
		return(LUAREST_ERROR);
    }
	lua_getglobal(ls, "luarest_init");
    if (lua_isfunction(ls, -1) == 0) {
		printf("Couln'd find 'luarest_init' table in LUA script!\n");
		lua_close(ls);
		return(LUAREST_ERROR);
	}
	app = (application*)lua_newuserdata(ls, sizeof(application));
	luaL_getmetatable(ls, LUA_USERDATA_APPLICATION);
	lua_setmetatable(ls, -2);
	app->s = NULL;
	utstring_new(app->name);
	utstring_printf(app->name, appName);
	if (lua_pcall(ls, 1, 0, 0) != 0) {
		printf("Error calling luarest_init: %s\n!", lua_tostring(ls, -1));
		lua_close(ls);
		return(LUAREST_ERROR);
	}
	app->lua_state = ls;
	HASH_ADD_KEYPTR(hh, *apps, utstring_body(app->name), utstring_len(app->name), app);
	return(LUAREST_SUCCESS);
}
/**
 *
 *
 */
static luarest_status parse_apps(application** apps, char* directory_path)
{
#ifdef WIN32
	WIN32_FIND_DATA ffd;
	WIN32_FIND_DATA fap;
	size_t length_of_arg;
	TCHAR szDir[MAX_PATH];
	TCHAR appFile[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;
	HANDLE hApp = INVALID_HANDLE_VALUE;
	luarest_status ret;
	// Check that the input path plus 2 is not longer than MAX_PATH.
	StringCchLength(directory_path, MAX_PATH, &length_of_arg);
	if (length_of_arg > (MAX_PATH - 2)) {
		_tprintf(TEXT("\nDirectory path is too long.\n"));
		return(LUAREST_ERROR);
	}
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.
	StringCchCopy(szDir, MAX_PATH, directory_path);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));
	// Find the first file in the directory.
	hFind = FindFirstFile(szDir, &ffd);
	if (INVALID_HANDLE_VALUE == hFind) {
      return(LUAREST_ERROR);
	}
	// List all the files in the directory with some info about them.
	do {
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			StringCchCopy(appFile, MAX_PATH, directory_path);
			StringCchCat(appFile, MAX_PATH, TEXT("\\"));
			StringCchCat(appFile, MAX_PATH, ffd.cFileName);
			StringCchCat(appFile, MAX_PATH, TEXT("\\"));
			StringCchCat(appFile, MAX_PATH, APP_ENTRY_POINT);
			hApp = FindFirstFile(appFile, &fap);
			if (hApp != INVALID_HANDLE_VALUE) {
				UT_string* app;
				utstring_new(app);
				utstring_printf(app, appFile);
				/* verify application */
				ret = verify_application(apps, ffd.cFileName, app);
				utstring_free(app);
				if (ret != LUAREST_SUCCESS) {
					printf("Application %s couldn't be load due to errors!\n", ffd.cFileName);
				}
			}
			FindClose(hApp);
		}
	}
	while (FindNextFile(hFind, &ffd) != 0);
	FindClose(hFind);
#else
    DIR *dpdf;
    struct dirent *epdf;
    struct stat st;
    int is_dir = 0;
    dpdf = opendir("./");
    if (dpdf != NULL){
        while ( (epdf = readdir(dpdf)) != NULL) {
            if (stat(epdf->d_name, &st) == -1) {
                continue;
            }
            is_dir = (st.st_mode & S_IFDIR) != 0;
            if (is_dir) {
                continue;
            }
            /* re-do for unix */
        }
    }
    closedir(dpdf);
#endif
	return(LUAREST_SUCCESS);
}
/**
 *
 *
 */
luarest_status invoke_application(application* apps, UT_string* url, luarest_method m, luarest_response* res_code, 
	luarest_content_type* con_type, UT_string* res_buf)
{
	application* app = NULL;
	service *service;
	char* pch = NULL;
	char* tmp = utstring_body(url);
	UT_string* app_name;
	UT_string* key;

	utstring_new(app_name);
	pch = strchr(++tmp, '/');
	if (pch == NULL) {
		return(LUAREST_ERROR);
	}
	utstring_bincpy(app_name, tmp, pch-tmp);
	HASH_FIND(hh, apps, utstring_body(app_name), utstring_len(app_name), app);
	utstring_free(app_name);
	if (app == NULL) {
		return(LUAREST_ERROR);
	}
	utstring_new(key);
	utstring_printf(key, "M%d#P%s", m, pch);
	HASH_FIND(hh, app->s, utstring_body(key), utstring_len(key), service);
	if (service == NULL) {
		return(LUAREST_ERROR);
	}
	invoke_lua(app->lua_state, service->callback_ref, res_code, con_type, res_buf);
	return(LUAREST_SUCCESS);
}
/**
 *
 *
 */
luarest_status create_applications(application** apps, char* app_dir)
{
	luarest_status ret = LUAREST_SUCCESS;

	ret = parse_apps(apps, app_dir);

	return(LUAREST_SUCCESS);
}
/**
 *
 *
 */
luarest_status free_applications(application* apps)
{
	return(LUAREST_SUCCESS);
}