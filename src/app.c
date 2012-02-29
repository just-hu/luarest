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

#include <lauxlib.h>
#include <lualib.h>

#include "app.h"

#define LUA_ENUM(L, name, val) \
  lua_pushlstring(L, #name, sizeof(#name)-1); \
  lua_pushnumber(L, val); \
  lua_settable(L, -3);

#define LUA_USERDATA_SERVICES "luarest.service"

/* forward decls */
static int l_service_register(lua_State* state);

static const struct luaL_Reg l_service [] = {
	{"register", l_service_register},
	{NULL, NULL} /* sentinel */
};

/**
 * LUA syntax: service.register(method, url, callback)
 *
 * Return: boolean true on success
 *
 */
static int l_service_register(lua_State* state)
{
	
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

		LUA_ENUM(state, HTTP_METHOD_GET, i++);
		LUA_ENUM(state, HTTP_METHOD_POST, i++);
		LUA_ENUM(state, HTTP_METHOD_PUT, i++);
		LUA_ENUM(state, HTTP_METHOD_DELETE, i++);
		LUA_ENUM(state, HTTP_METHOD_OPTION, i++);
		LUA_ENUM(state, HTTP_METHOD_HEAD, i++);

		/* register  */
		i = 1;
		LUA_ENUM(state, HTTP_RESPONSE_OK, i++);
		LUA_ENUM(state, HTTP_RESPONSE_CREATED, i++);
		LUA_ENUM(state, HTTP_RESPONSE_NO_CONTENT, i++);
		LUA_ENUM(state, HTTP_RESPONSE_NOT_ACCEPTABLE, i++);
		LUA_ENUM(state, HTTP_RESPONSE_NOT_MODIFIED, i++);
		LUA_ENUM(state, HTTP_RESPONSE_SEE_OTHER, i++);
		LUA_ENUM(state, HTTP_RESPONSE_SERVER_ERROR, i++);
		LUA_ENUM(state, HTTP_RESPONSE_TEMPORARY_REDIRECT, i++);
	}
	
	luaL_newmetatable(state, LUA_USERDATA_SERVICES);
	lua_pushvalue(state, -1);
	lua_setfield(state, -2, "__index");
	luaL_register(state, NULL, l_service);

	return(1);
}
/**
 *
 *
 */
static luarest_status verify_application(application* apps, UT_string* path)
{
	int ret;
	lua_State* ls = luaL_newstate();
	
	luaL_openlibs(ls);
	luaopen_luarestlibs(ls);
        
    ret = luaL_loadfile(ls, utstring_body(path));
    if (ret != 0) {
		printf("Couldn't load file: %s\n", lua_tostring(ls, -1));
		lua_close(ls);
		return(LUAREST_ERROR);
    }
	ret = lua_pcall(ls, 0, 0, 0);
    if (ls != 0) {
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
	
	/*lua_bid = (market_data_candle*)lua_newuserdata(s->state, sizeof(market_data_candle));
	luaL_getmetatable(s->state, LUA_USERDATA_MARKETDATA_CANDLE);
	lua_setmetatable(s->state, -2);
	lua_ask = (market_data_candle*)lua_newuserdata(s->state, sizeof(market_data_candle));
	luaL_getmetatable(s->state, LUA_USERDATA_MARKETDATA_CANDLE);
	lua_setmetatable(s->state, -2);
	s->context->market_data->copy_market_data_candle(bid_candle, lua_bid);
	s->context->market_data->copy_market_data_candle(ask_candle, lua_ask);
	if (lua_pcall(s->state, 3, 0, 0) != 0) {
		printf("Error calling market_data: %s\n!", lua_tostring(s->state, -1));
	}*/

	return(LUAREST_SUCCESS);
}
/**
 *
 *
 */
static luarest_status parse_apps(application* apps, char* directory_path)
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
				ret = verify_application(apps, app);
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
            ch_array_push_back(files, &epdf->d_name);
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
luarest_status create_applications(application* apps, char* app_dir)
{
	luarest_status ret = LUAREST_SUCCESS;

	ret = parse_apps(apps, app_dir);

	//(*apps)->lua_state = luaL_newstate();

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