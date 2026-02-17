/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2026, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebFileSystem.h"

#define LUA_LIB

#include "lua.hpp"
#include <cstring>

static const char* LUA_WFSHANDLE = "wfs:File*";
static const char* DIR_METATABLE = "directory metatable";

struct WFSStream
{
    fs::File f;  /* stream (NULL for incompletely created streams) */
    lua_CFunction closef;  /* to close stream (NULL for closed streams) */
};

struct dir_data
{
    bool closed;
    fs::File dir;
};

static inline WFSStream* towfsstream(lua_State* L)	{ return reinterpret_cast<WFSStream*>(luaL_checkudata(L, 1, LUA_WFSHANDLE)); }
static inline bool isclosed(WFSStream* p) { return p->closef == nullptr; }

/* Check whether 'mode' matches '[rwa]%+?[L_MODEEXT]*' */
static int l_checkmode(const char* mode)
{
    return (*mode != '\0' && strchr("rwa", *(mode++)) != nullptr &&
         (*mode != '+' || ((void)(++mode), 1)));
}

static int f_tostring (lua_State *L)
{
    WFSStream* p = towfsstream(L);
    if (isclosed(p)) {
        lua_pushliteral(L, "file (closed)");
    } else {
        lua_pushfstring(L, "file (%p)", p->f.filePtr());
    }
    return 1;
}

static fs::File& tofile(lua_State *L)
{
    WFSStream* p = towfsstream(L);
    if (l_unlikely(isclosed(p))) {
        luaL_error(L, "attempt to use a closed file");
    }
    lua_assert(p->f.isOpenFile());
    return p->f;
}

/*
** When creating file handles, always creates a 'closed' file handle
** before opening the actual file; so, if there is a memory error, the
** handle is in a consistent state.
*/
static WFSStream* newprefile(lua_State* L)
{
    WFSStream* p = reinterpret_cast<WFSStream*>(lua_newuserdatauv(L, sizeof(WFSStream), 0));
    p->closef = nullptr;  /* mark file handle as 'closed' */
    
    // Since the fs::File is a c++ object we need to do a placement new on it
    new (&(p->f)) fs::File();
    
    luaL_setmetatable(L, LUA_WFSHANDLE);
    return p;
}

/*
** Calls the 'close' function from a file handle. The 'volatile' avoids
** a bug in some versions of the Clang compiler (e.g., clang 3.0 for
** 32 bits).
*/
static int aux_close(lua_State *L)
{
    WFSStream* p = towfsstream(L);
    volatile lua_CFunction cf = p->closef;
    p->closef = nullptr;  /* mark stream as closed */
    return (*cf)(L);  /* close it */
}

static int f_close(lua_State* L)
{
    tofile(L);  /* make sure argument is an open stream */
    return aux_close(L);
}


static int f_gc (lua_State* L)
{
    WFSStream* p = towfsstream(L);
    if (!isclosed(p)) {
        aux_close(L);  /* ignore closed and incompletely open files */
    }
    
    // fs::File is a c++ object, but there's no need to call the dtor
    // since it's just been closed
    return 0;
}

/*
** function to close regular files
*/
static int io_fclose(lua_State* L)
{
    WFSStream* p = towfsstream(L);
    errno = 0;
    return luaL_fileresult(L, p->f.close(), nullptr);
}

static WFSStream* newfile(lua_State* L)
{
    WFSStream* p = newprefile(L);
    p->closef = &io_fclose;
    return p;
}

static int io_exists(lua_State* L)
{
    const char* filename = luaL_checkstring(L, 1);
    errno = 0;
    lua_pushboolean(L, mil::WebFileSystem::exists(filename));
    return 1;
}

static int io_open(lua_State* L)
{
    const char* filename = luaL_checkstring(L, 1);
    const char* mode = luaL_optstring(L, 2, "r");
    WFSStream* p = newfile(L);
    const char* md = mode;  /* to traverse/check mode */
    luaL_argcheck(L, l_checkmode(md), 2, "invalid mode");
    errno = 0;
    p->f = mil::WebFileSystem::open(filename, mode);
    return (!p->f.isOpenFile()) ? luaL_fileresult(L, 0, filename) : 1;
}

static int io_remove (lua_State* L)
{
    const char *filename = luaL_checkstring(L, 1);
    errno = 0;
    return luaL_fileresult(L, mil::WebFileSystem::remove(filename), filename);
}


static int io_rename (lua_State* L)
{
    const char *fromname = luaL_checkstring(L, 1);
    const char *toname = luaL_checkstring(L, 2);
    errno = 0;
    return luaL_fileresult(L, mil::WebFileSystem::rename(fromname, toname), nullptr);
}

static int io_mkdir(lua_State* L)
{
    const char *path = luaL_checkstring(L, 1);
    errno = 0;
    return luaL_fileresult(L, mil::WebFileSystem::mkdir(path), path);
}

static int io_rmdir(lua_State* L)
{
    const char *path = luaL_checkstring(L, 1);
    return luaL_fileresult(L, mil::WebFileSystem::rmdir(path), path);
}

static int io_totalbytes(lua_State* L)
{
    lua_pushinteger(L, lua_Integer(mil::WebFileSystem::totalBytes()));
    return 1;
}

static int io_usedbytes(lua_State* L)
{
    lua_pushinteger(L, lua_Integer(mil::WebFileSystem::usedBytes()));
    return 1;
}

static int io_dir_iter(lua_State* L)
{
//    dir_data* d = reinterpret_cast<dir_data *>(luaL_checkudata(L, 1, DIR_METATABLE));
//    luaL_argcheck(L, d->closed == 0, 1, "closed directory");
//
//    if ((entry = readdir(d->dir)) != NULL) {
//        lua_pushstring(L, entry->d_name);
//        return 1;
//    } else {
//        /* no more entries => close directory */
//        closedir(d->dir);
//        d->closed = 1;
//        return 0;
//    }
    return 0;
}

static int io_dir_iter_factory(lua_State* L)
{
    const char *path = luaL_checkstring(L, 1);
    lua_pushcfunction(L, io_dir_iter);
    dir_data* d = reinterpret_cast<dir_data*>(lua_newuserdata(L, sizeof(dir_data)));
    luaL_getmetatable(L, DIR_METATABLE);
    lua_setmetatable(L, -2);
    d->closed = false;
    d->dir = mil::WebFileSystem::open(path);
    if (!d->dir) {
        luaL_error(L, "cannot open %s: %s", path, strerror(errno));
    } else if (!d->dir.isDirectory()) {
        luaL_error(L, "%s is not a directory", path);
    }
    lua_pushnil(L);
    lua_pushvalue(L, -2);
    return 4;
}

static int f_seek(lua_State* L)
{
    static fs::SeekMode mode[] = { fs::SeekMode::Set, fs::SeekMode::Cur, fs::SeekMode::End };
    static const char *const modenames[] = {"set", "cur", "end", NULL};
    fs::File& f = tofile(L);
    int op = luaL_checkoption(L, 2, "cur", modenames);
    lua_Integer p3 = luaL_optinteger(L, 3, 0);
    uint32_t offset = uint32_t(p3);
    luaL_argcheck(L, (lua_Integer)offset == p3, 3, "not an integer in proper range");
    errno = 0;
    op = f.seek(offset, mode[op]);
    if (l_unlikely(op)) {
        return luaL_fileresult(L, 0, nullptr);  /* error */
    } else {
        lua_pushinteger(L, (lua_Integer)f.position());
    }
    return 1;
}

static int f_name(lua_State* L)
{
    fs::File& f = tofile(L);
    errno = 0;
    if (!f) {
        return luaL_fileresult(L, 0, nullptr);
    }
    lua_pushstring(L, f.name());
    return 1;
}

static int test_eof(lua_State *L, fs::File& f)
{
    int c = f.peek();
    lua_pushliteral(L, "");
    return (c != EOF);
}

static int read_line(lua_State* L, fs::File& f, bool chop)
{
    luaL_Buffer b;
    int c = 0;
    luaL_buffinit(L, &b);
    
    do {  /* may need to read several chunks to get whole line */
        char *buff = luaL_prepbuffer(&b);  /* preallocate buffer space */
        int i = 0;
        while (i < LUAL_BUFFERSIZE && (c = f.read()) != EOF && c != '\n') {
            buff[i++] = c;  /* read up to end of line or buffer limit */
        }
        luaL_addsize(&b, i);
    } while (c != EOF && c != '\n');  /* repeat until end of line */
    
    if (!chop && c == '\n') {  /* want a newline and have one? */
        luaL_addchar(&b, c);  /* add ending newline to result */
    }
    
    luaL_pushresult(&b);  /* close buffer */
    /* return ok if read something (either a newline or something else) */
    return (c == '\n' || lua_rawlen(L, -1) > 0);
}

static int read_chars(lua_State* L, fs::File& f, size_t n)
{
    size_t nr;  /* number of chars actually read */
    uint8_t* p;
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    p = reinterpret_cast<uint8_t*>(luaL_prepbuffsize(&b, n));  /* prepare buffer to read whole block */
    nr = f.read(p, n);  /* try to read 'n' chars */
    luaL_addsize(&b, nr);
    luaL_pushresult(&b);  /* close buffer */
    return (nr > 0);  /* true iff read something */
}

static int g_read(lua_State* L, fs::File& f, int first)
{
    int nargs = lua_gettop(L) - 1;
    int n, success;
    f.clearError();
    errno = 0;
    
    if (nargs == 0) {  /* no arguments? */
        success = read_line(L, f, 1);
        n = first + 1;  /* to return 1 result */
    } else {
        /* ensure stack space for all results and for auxlib's buffer */
        luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
        success = 1;
        for (n = first; nargs-- && success; n++) {
            if (lua_type(L, n) == LUA_TNUMBER) {
                size_t l = (size_t)luaL_checkinteger(L, n);
                success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
            } else {
                const char *p = luaL_checkstring(L, n);
                if (*p == '*') {
                    p++;  /* skip optional '*' (for compatibility) */
                }
                
                switch (*p) {
                    case 'l':  /* line */
                        success = read_line(L, f, 1);
                        break;
                    case 'L':  /* line with end-of-line */
                        success = read_line(L, f, 0);
                        break;
                    default:
                        return luaL_argerror(L, n, "invalid format");
                }
            }
        }
    }
    
    if (f.error()) {
        return luaL_fileresult(L, 0, NULL);
    }
    if (!success) {
        lua_pop(L, 1);  /* remove last result */
        luaL_pushfail(L);  /* push nil instead */
    }
    return n - first;
}

static int f_read(lua_State* L)
{
    return g_read(L, tofile(L), 2);
}

// We support writing strings only
static int g_write(lua_State* L, fs::File& f, int arg)
{
    int nargs = lua_gettop(L) - arg;
    int status = 1;
    errno = 0;
    for ( ; nargs--; arg++) {
        size_t l;
        const uint8_t* s = reinterpret_cast<const uint8_t*>(luaL_checklstring(L, arg, &l));
        status = status && (f.write(s, l) == l);
    }
    
    if (l_likely(status)) {
        return 1;  /* file handle already on stack top */
    } else {
        return luaL_fileresult(L, status, NULL);
    }
}

static int f_write(lua_State* L)
{
    fs::File& f = tofile(L);
    lua_pushvalue(L, 1);  /* push file at the stack top (to be returned) */
    return g_write(L, f, 2);
}

// peek at the next char and return it
static int f_peek(lua_State* L)
{
    fs::File& f = tofile(L);
    errno = 0;
    if (!f) {
        return luaL_fileresult(L, 0, nullptr);
    }
    lua_pushnumber(L, f.peek());
    return 1;
}

static int f_flush(lua_State* L)
{
    fs::File& f = tofile(L);
    errno = 0;
    return luaL_fileresult(L, f.flush(), NULL);
}

static int f_position(lua_State* L)
{
    fs::File& f = tofile(L);
    errno = 0;
    if (!f) {
        return luaL_fileresult(L, 0, nullptr);
    }
    lua_pushinteger(L, lua_Integer(f.position()));
    return 1;
}

static int f_size(lua_State* L)
{
    fs::File& f = tofile(L);
    errno = 0;
    if (!f) {
        return luaL_fileresult(L, 0, nullptr);
    }
    lua_pushinteger(L, lua_Integer(f.size()));
    return 1;
}

static int f_isdir(lua_State* L)
{
    fs::File& f = tofile(L);
    errno = 0;
    if (!f) {
        return luaL_fileresult(L, 0, nullptr);
    }
    lua_pushboolean(L, lua_Integer(f.isDirectory()));
    return 1;
}

/*
** functions for 'wfs' library
*/
static const luaL_Reg wfslib[] = {
  {"exists", io_exists},
  {"open", io_open},
  {"remove", io_remove},
  {"rmdir", io_rmdir},
  {"mkdir", io_mkdir},
  {"rename", io_rename},
  {"total_bytes", io_totalbytes},
  {"used_bytes", io_usedbytes},
  {"dir", io_dir_iter_factory},
  {NULL, NULL}
};


/*
** methods for file handles
*/
static const luaL_Reg meth[] = {
  {"name", f_name},
  {"read", f_read},
  {"write", f_write},
  {"flush", f_flush},
  {"peek", f_peek},
  {"position", f_position},
  {"seek", f_seek},
  {"size", f_size},
  {"isdir", f_isdir},
  {"close", f_close},
  {NULL, NULL}
};

/*
** metamethods for file handles
*/
static const luaL_Reg metameth[] = {
  {"__index", NULL},  /* placeholder */
  {"__gc", f_gc},
  {"__close", f_gc},
  {"__tostring", f_tostring},
  {NULL, NULL}
};

static void createmeta (lua_State* L)
{
    luaL_newmetatable(L, LUA_WFSHANDLE);  /* metatable for file handles */
    luaL_setfuncs(L, metameth, 0);  /* add metamethods to new metatable */
    luaL_newlibtable(L, meth);  /* create method table */
    luaL_setfuncs(L, meth, 0);  /* add file methods to method table */
    lua_setfield(L, -2, "__index");  /* metatable.__index = method table */
    lua_pop(L, 1);  /* pop metatable */
}

extern "C" {
LUAMOD_API int luaopen_wfs(lua_State* L)
{
    luaL_newlib(L, wfslib);  /* new module */
    createmeta(L);
    return 1;
}
}
