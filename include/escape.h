#ifndef __LUAREST_ESCAPE_H__
#define __LUAREST_ESCAPE_H__

#include "luarest.h"

/*-----------------------------------------------------------------------------
 * Functions prototypes
 *----------------------------------------------------------------------------*/
luarest_status url_escape(const char* src, char* target);
luarest_status url_unescape(const char* src, char* target);

#endif