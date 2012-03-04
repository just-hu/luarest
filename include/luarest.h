#ifndef __LUAREST_H__
#define __LUATEST_H__

#ifdef WIN32
#include <windows.h>
#ifndef __cplusplus
typedef int bool;
#define true 1
#define false 0
#endif
#pragma warning( once : 4996 4244 )
#else
#include <stddef.h>
#include <stdbool.h>
#endif
#include <math.h>
#include <assert.h>

/*-----------------------------------------------------------------------------
 * Constants
 *----------------------------------------------------------------------------*/
#define LUAREST_SUCCESS 0
#define LUAREST_ERROR   1

/*-----------------------------------------------------------------------------
 * Macros
 *----------------------------------------------------------------------------*/
#ifdef WIN32
#define MAX(a,b) max(a,b)
#define MIN(a,b) min(a,b)
#else
#define MAX(a,b) fmax(a,b)
#define MIN(a,b) fmin(a,b)
#endif

#define inline __inline

#define strcasecmp stricmp

/*-----------------------------------------------------------------------------
 * Data types
 *----------------------------------------------------------------------------*/
typedef int luarest_status;

#endif