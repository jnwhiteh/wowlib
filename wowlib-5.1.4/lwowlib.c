/*
** This file implements lua API aliases (and a few specialized functions)
** specific to WoW.
*/

#define LUA_LIB

#include <string.h>
#include <stdlib.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static int wow_strtrim(lua_State *L)
{
	int front = 0, back;
	const char *str = luaL_checklstring(L, 1, (size_t *)(&back));
	const char *del = luaL_optstring(L, 2, "\t\n\r ");
	--back;

	while (front <= back && strchr(del, str[front]))
		++front;
	while (back > front && strchr(del, str[back]))
		--back;
	
	lua_pushlstring(L, &str[front], back - front + 1);
	return 1;
}

/* strsplit & strjoin adapted from code by Norganna */
static int wow_strsplit(lua_State *L)
{
	const char *sep = luaL_checkstring(L, 1);
	const char *str = luaL_checkstring(L, 2);
	int limit = luaL_optint(L, 3, 0);
	int count = 0;
	/* Set the stack to a predictable size */
	lua_settop(L, 0);
	/* Initialize the result count */
	/* Tokenize the string */
	if(!limit || limit > 1) {
		const char *end = str;
		while(*end) {
			int issep = 0;
			const char *s = sep;
			for(; *s; ++s) {
				if(*s == *end) {
					issep = 1;
					break;
				}
			}
			if(issep) {
				luaL_checkstack(L, count+1, "too many results");
				lua_pushlstring(L, str, (end-str));
				++count;
				str = end+1;
				if(count == (limit-1)) {
					break;
				}
			}
			++end;
		}
	}
	/* Add the remainder */
	luaL_checkstack(L, count+1, "too many results");
	lua_pushstring(L, str);
	++count;
	/* Return with the number of values found */
	return count;
}

static int wow_strjoin(lua_State *L)
{
	size_t seplen;
	int entries;
	const char *sep = luaL_checklstring(L, 1, &seplen);
	
	/* Guarantee we have 1 stack slot free */
	lua_remove(L, 1);

	entries = lua_gettop(L);

	if (seplen == 0) /* If there's no seperator, then this is the same as a concat */
		lua_concat(L, entries);
	else if (entries == 0) /* If there are no entries then we can't concatenate anything */
		lua_pushstring(L, "");
	else if (entries == 1) /* If there's only one entry, just return it */
		;
	else {
		luaL_Buffer b;
		int i;

		/* Set up buffer to store resulting string */
		luaL_buffinit(L, &b);
		for(i = 1; i <= entries; ++i) {
			/* Push the current entry and add it to the buffer */
			lua_pushvalue(L, i);
			luaL_addvalue(&b);
			/* Add the separator to the buffer */
			if (i < entries) {
				luaL_addlstring(&b, sep, seplen);
			}
		}
		luaL_pushresult(&b);
	}

	return 1;
}

static int wow_strconcat(lua_State *L)
{
	lua_concat(L, lua_gettop(L));
	return 1;
}

static int wow_strreplace(lua_State *L)
{
	const char *subject = luaL_checkstring(L, 1);
	const char *search = luaL_checkstring(L, 2);
	const char *replace = luaL_checkstring(L, 3);

	char *replaced = (char*)calloc(1, 1), *temp = NULL;
	const char *p = subject, *p3 = subject, *p2;
	int  found = 0;
	int count = 0;

	while ( (p = strstr(p, search)) != NULL) {
		found = 1;
		count++;
		temp = realloc(replaced, strlen(replaced) + (p - p3) + strlen(replace) + 1);
		if (temp == NULL) {
			free(replaced);
			luaL_error(L, "Unable to allocate memory");
			return 0;
		}
		replaced = temp;
		strncat(replaced, p - (p - p3), p - p3);
		strcat(replaced, replace);
		p3 = p + strlen(search);
		p += strlen(search);
		p2 = p;
	}

	if (found == 1) {
		if (strlen(p2) > 0) {
			temp = realloc(replaced, strlen(replaced) + strlen(p2) + 1);
			if (temp == NULL) {
				free(replaced);
				luaL_error(L, "Unable to allocate memory");
				return 0;
			}
			replaced = temp;
			strcat(replaced, p2);
		}
	} else {
		temp = realloc(replaced, strlen(subject) + 1);
		if (temp != NULL) {
			replaced = temp;
			strcpy(replaced, subject);
		}
	}

	lua_pushstring(L, replaced);
	lua_pushinteger(L, count);
	return 2;
}

static int wow_getglobal(lua_State *L)
{
	lua_getglobal(L, luaL_checkstring(L, 1));
	return 1;
}

static int wow_setglobal(lua_State *L)
{
	lua_settop(L, 2);
	lua_setglobal(L, luaL_checkstring(L, 1));
	return 0;
}

static int wow_debugstack(lua_State *L)
{
	int start = luaL_optint(L, 1, 1);
	/*int ntop = luaL_optint(L, 2, 12); Ignored for now... May implement more accurate version in the future
	int nbot = luaL_optint(L, 3, 10);*/
	const char *trace;
	size_t tracelen;

	lua_getglobal(L, "debug");
	lua_pushstring(L, "traceback");
	lua_gettable(L, -2);
	lua_pushstring(L, "");
	lua_pushnumber(L, start);

	if (lua_pcall(L, 2, 1, 0) != 0)
		luaL_error(L, "error running `debug.traceback': %s", lua_tostring(L, -1));

	if (!lua_isstring(L, -1))
		luaL_error(L, "function `debug.traceback' did not return a string");

	trace = lua_tolstring(L, -1, &tracelen);

	/* TODO: Implement head/tail code here

	lua_checkstack(L, 1);
	lua_pushstring(L, ret);	*/

	return 1;
}

static int wow_scrub(lua_State *L)
{
	int entries = lua_gettop(L);
	int cur;

	for (cur = 1; cur <= entries; cur++) {
		switch (lua_type(L, cur)) {
			case LUA_TTABLE:
			case LUA_TFUNCTION:
			case LUA_TUSERDATA:
			case LUA_TTHREAD:
			case LUA_TLIGHTUSERDATA:
				lua_pushnil(L);
				lua_replace(L, cur);
				break;
		}
	}

	return entries;
}

static int wow_tostringall(lua_State *L)
{
	int entries = lua_gettop(L);
	int cur;

	for (cur = 1; cur <= entries; cur++) {
		if (luaL_callmeta(L, cur, "__tostring"))
			lua_replace(L, cur);
		switch (lua_type(L, cur)) {
			case LUA_TNUMBER:
				lua_pushstring(L, lua_tostring(L, cur));
				lua_replace(L, cur);
				break;
			case LUA_TSTRING:
				break;
			case LUA_TBOOLEAN:
				lua_pushstring(L, (lua_toboolean(L, cur) ? "true" : "false"));
				lua_replace(L, cur);
				break;
			case LUA_TNIL:
				lua_pushliteral(L, "nil");
				lua_replace(L, cur);
				break;
			default:
				lua_pushfstring(L, "%s: %p", luaL_typename(L, cur), lua_topointer(L, cur));
				lua_replace(L, cur);
				break;
		}
	}

	return entries;
}

static int wow_wipe(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE); 

	lua_pushnil(L);
	while (lua_next(L, 1) != 0) {
		lua_pop(L, 1);
		lua_pushvalue(L, -1);
		lua_pushnil(L);
		lua_settable(L, 1);
	}

	return 1;
}

/* Implementation of string.format, taken from lstrlib.c */

#define L_ESC		'%'
#define L_POS		'$'
/* macro to `unsign' a character */
#define uchar(c)        ((unsigned char)(c))

/* maximum size of each formatted item (> len(format('%99.99f', -1e308))) */
#define MAX_ITEM	512
/* valid flags in a format specification */
#define FLAGS	"-+ #0"
/*
** maximum size of each format specification (such as '%-099.99d')
** (+10 accounts for %99.99x plus margin of error)
*/
#define MAX_FORMAT	(sizeof(FLAGS) + sizeof(LUA_INTFRMLEN) + 10)


static void addquoted (lua_State *L, luaL_Buffer *b, int arg) {
  size_t l;
  const char *s = luaL_checklstring(L, arg, &l);
  luaL_addchar(b, '"');
  while (l--) {
    switch (*s) {
      case '"': case '\\': case '\n': {
        luaL_addchar(b, '\\');
        luaL_addchar(b, *s);
        break;
      }
      case '\r': {
        luaL_addlstring(b, "\\r", 2);
        break;
      }
      case '\0': {
        luaL_addlstring(b, "\\000", 4);
        break;
      }
      default: {
        luaL_addchar(b, *s);
        break;
      }
    }
    s++;
  }
  luaL_addchar(b, '"');
}

static const char *scanformat (lua_State *L, const char *strfrmt, char *form) {
  const char *p = strfrmt;
  while (*p != '\0' && strchr(FLAGS, *p) != NULL) p++;  /* skip flags */
  if ((size_t)(p - strfrmt) >= sizeof(FLAGS))
    luaL_error(L, "invalid format (repeated flags)");
  if (isdigit(uchar(*p))) p++;  /* skip width */
  if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
  if (*p == '.') {
    p++;
    if (isdigit(uchar(*p))) p++;  /* skip precision */
    if (isdigit(uchar(*p))) p++;  /* (2 digits at most) */
  }
  if (isdigit(uchar(*p)))
    luaL_error(L, "invalid format (width or precision too long)");
  *(form++) = '%';
  strncpy(form, strfrmt, p - strfrmt + 1);
  form += p - strfrmt + 1;
  *form = '\0';
  return p;
}

static void addintlen (char *form) {
  size_t l = strlen(form);
  char spec = form[l - 1];
  strcpy(form + l - 1, LUA_INTFRMLEN);
  form[l + sizeof(LUA_INTFRMLEN) - 2] = spec;
  form[l + sizeof(LUA_INTFRMLEN) - 1] = '\0';
}

static int wow_strformat (lua_State *L) {
  int arg = 1;
  size_t sfl;
  const char *strfrmt = luaL_checklstring(L, arg, &sfl);
  const char *strfrmt_end = strfrmt+sfl;
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  while (strfrmt < strfrmt_end) {
    if (*strfrmt != L_ESC)
      luaL_addchar(&b, *strfrmt++);
    else if (*++strfrmt == L_ESC)
      luaL_addchar(&b, *strfrmt++);  /* %% */
    else { /* format item */
      char form[MAX_FORMAT];  /* to store the format (`%...') */
      char buff[MAX_ITEM];  /* to store the formatted item */
	  /* Added back the lua 4.0 feature to choose which argument to format */
	  const char *initf = strfrmt;
	  if (isdigit((unsigned char)*initf) && *(initf+1) == L_POS) {
		  arg = *initf - '0';
		  initf += 2;
	  }
      arg++;
      strfrmt = scanformat(L, initf, form);
      switch (*strfrmt++) {
        case 'c': {
          sprintf(buff, form, (int)luaL_checknumber(L, arg));
          break;
        }
        case 'd':  case 'i': {
          addintlen(form);
          sprintf(buff, form, (LUA_INTFRM_T)luaL_checknumber(L, arg));
          break;
        }
        case 'o':  case 'u':  case 'x':  case 'X': {
          addintlen(form);
          sprintf(buff, form, (unsigned LUA_INTFRM_T)luaL_checknumber(L, arg));
          break;
        }
        case 'e':  case 'E': case 'f':
        case 'g': case 'G': {
          sprintf(buff, form, (double)luaL_checknumber(L, arg));
          break;
        }
        case 'q': {
          addquoted(L, &b, arg);
          continue;  /* skip the 'addsize' at the end */
        }
        case 's': {
          size_t l;
          const char *s = luaL_checklstring(L, arg, &l);
          if (!strchr(form, '.') && l >= 100) {
            /* no precision and string is too long to be formatted;
               keep original string */
            lua_pushvalue(L, arg);
            luaL_addvalue(&b);
            continue;  /* skip the `addsize' at the end */
          }
          else {
            sprintf(buff, form, s);
            break;
          }
        }
        default: {  /* also treat cases `pnLlh' */
          return luaL_error(L, "invalid option " LUA_QL("%%%c") " to "
                               LUA_QL("format"), *(strfrmt - 1));
        }
      }
      luaL_addlstring(&b, buff, strlen(buff));
    }
  }
  luaL_pushresult(&b);
  return 1;
}


static const struct luaL_reg wowlib[] = {
	{"strformat",	wow_strformat},
	{"strtrim",		wow_strtrim},
	{"strsplit",	wow_strsplit},
	{"strjoin",		wow_strjoin},
	{"strconcat",	wow_strconcat},
	{"strreplace",  wow_strreplace},
	{"getglobal",	wow_getglobal},
	{"setglobal",	wow_setglobal},
	{"debugstack",	wow_debugstack},
	{"scrub",		wow_scrub},
	{"tostringall", wow_tostringall},
	{"wipe",		wow_wipe},
	{NULL,			NULL}
};

/* This is a lua chunk that sets up the global aliases to library functions */
static const char *aliases =
	"----------- STRING FORMAT ----------\n"
	"string.format = wow.strformat\n"

	"---------------- os ----------------\n"
	"date = os.date\n"
	"time = os.time\n"
	"difftime = os.difftime\n"

	"---------------- math ----------------\n"
	"abs = math.abs\n"
	"acos = function (x) return math.deg(math.acos(x)) end\n"
	"asin = function (x) return math.deg(math.asin(x)) end\n"
	"atan = function (x) return math.deg(math.atan(x)) end\n"
	"atan2 = function (x,y) return math.deg(math.atan2(x,y)) end\n"
	"ceil = math.ceil\n"
	"cos = function (x) return math.cos(math.rad(x)) end\n"
	"deg = math.deg\n"
	"exp = math.exp\n"
	"floor = math.floor\n"
	"frexp = math.frexp\n"
	"ldexp = math.ldexp\n"
	"log = math.log\n"
	"log10 = math.log10\n"
	"max = math.max\n"
	"min = math.min\n"
	"mod = math.fmod\n"
	"PI = math.pi\n"
	"rad = math.rad\n"
	"random = math.random\n"
	"randomseed = math.randomseed\n"
	"sin = function (x) return math.sin(math.rad(x)) end\n"
	"sqrt = math.sqrt\n"
	"tan = function (x) return math.tan(math.rad(x)) end\n"

	"---------------- string ----------------\n"
	"format = string.format\n"
	"gmatch = string.gmatch\n"
	"gsub = string.gsub\n"
	"strbyte = string.byte\n"
	"strchar = string.char\n"
	"strfind = string.find\n"
	"strlen = string.len\n"
	"strlower = string.lower\n"
	"strmatch = string.match\n"
	"strrep = string.rep\n"
	"strrev = string.reverse\n"
	"strsub = string.sub\n"
	"strupper = string.upper\n"

	"---------------- table ----------------\n"
	"foreach = table.foreach\n"
	"foreachi = table.foreachi\n"
	"getn = table.getn\n"
	"sort = table.sort\n"
	"tinsert = table.insert\n"
	"tremove = table.remove\n"

	"---------------- wow ----------------\n"
	"strtrim = wow.strtrim\n"
	"strsplit = wow.strsplit\n"
	"strjoin = wow.strjoin\n"
	"strconcat = wow.strconcat\n"
	"getglobal = wow.getglobal\n"
	"setglobal = wow.setglobal\n"
	"debugstack = wow.debugstack\n"
	"string.trim = wow.strtrim\n"
	"string.split = wow.strsplit\n"
	"string.join = wow.strjoin\n"
	"string.format = wow.strformat\n";

LUALIB_API int luaopen_wow(lua_State *L)
{
	/* Set up new functions */
	luaL_register(L, "wow", wowlib);
	
	/* Set up aliases */
	luaL_loadstring(L, aliases);
	lua_call(L, 0, 0);

	return 0;
}
