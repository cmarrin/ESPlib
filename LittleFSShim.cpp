/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "LittleFSShim.h"

File::File(const std::filesystem::path& path, const char* mode)
{
    _path = path;
    if (std::filesystem::is_directory(_path)) {
        _isDir = true;
        _dir = std::filesystem::directory_iterator(_path);
        _good = true;
    } else {
        _isDir = false;

        std::ios_base::openmode iosMode = std::ios::in;
        
        // FIXME: Need to deal the 'w' modes (erase contents) and append stuff
        std::string m = mode;
        if (m == "r+") {
            iosMode = std::ios::in | std::ios::out;
        } else if (m == "w") {
            iosMode = std::ios::out | std::ios::trunc | std::ios::binary;
        } else if (m == "w+") {
            iosMode = std::ios::in | std::ios::out | std::ios::trunc;
        } else if (m == "a") {
            iosMode = std::ios::out | std::ios::ate | std::ios::app;
        } else if (m == "a+") {
            iosMode = std::ios::in | std::ios::out | std::ios::app;
        }
        
        iosMode |=  std::ios::binary;
        
        _file = std::fstream(_path, iosMode);
        _good = _file.good();
    }
}

size_t
File::write(const uint8_t* buf, size_t size)
{
    if (_isDir) {
        return -1;
    }
    
    _file.write(reinterpret_cast<const char*>(buf), size);
    return _file.good() ? size : -1;
}

size_t
File::write(uint8_t c)
{
    if (_isDir) {
        return -1;
    }
    
    _file.write(reinterpret_cast<const char*>(&c), 1);
    return _file.good() ? 1 : -1;
}

size_t
File::read(uint8_t* buf, size_t size)
{
    if (_isDir) {
        return -1;
    }
    
    _file.read(reinterpret_cast<char*>(buf), size);
    return _file.good() ? size : -1;
}

int
File::read()
{
    if (_isDir) {
        return -1;
    }
    
    uint8_t c;
    _file.read(reinterpret_cast<char*>(&c), 1);
    return _file.good() ? c : -1;
}

int
File::peek()
{
    if (_isDir) {
        return -1;
    }
    
    int c = _file.peek();
    return _file.good() ? c : -1;
}

void
File::flush()
{
    if (_isDir) {
        return;
    }
    
    _file.flush();
}

bool
File::seek(uint32_t pos, SeekMode mode)
{
    if (_isDir) {
        return -1;
    }
    
    std::ios_base::seekdir dir;
    switch(mode) {
        default     : dir = std::ios_base::beg; break;
        case SeekCur: dir = std::ios_base::cur; break;
        case SeekEnd: dir = std::ios_base::end; break;
    }
    _file.seekp(pos, dir);
    return _file.good();
}

size_t
File::position() const
{
    if (_isDir) {
        return -1;
    }
    
    return const_cast<File*>(this)->_file.tellp();
}

size_t
File::size() const
{
    if (_isDir) {
        return -1;
    }
    
    File* self = const_cast<File*>(this);
    uint32_t pos = uint32_t(position());
    self->seek(0, SeekEnd);
    size_t size = position();
    self->seek(pos, SeekSet);
    return size;
}

void
File::close()
{
    _file.close();
}

const char*
File::path() const
{
    return _path.c_str();
}

const char*
File::name() const
{
    return _path.filename().c_str();
}

bool
File::isDirectory()
{
    return _isDir;
}

String
File::getNextFileName()
{
    bool b;
    return getNextFileName(&b);
}

String
File::getNextFileName(bool* isDir)
{
    if (!_isDir) {
        return "";
    }
    
    std::filesystem::path path = _dir->path();
    *isDir = _dir->is_directory();
    _dir++;
    return path.filename().c_str();
}

void
File::rewindDirectory()
{
    if (!_isDir) {
        return;
    }
    _dir = std::filesystem::directory_iterator(_path);
}

File
FS::open(const char* path, const char* mode, bool create)
{
    return File(makePath(path), mode);
}

bool
FS::exists(const char* path)
{
    return std::filesystem::exists(makePath(path));
}

bool
FS::remove(const char* path)
{
    return std::filesystem::remove(makePath(path));
}

bool
FS::rename(const char* fromPath, const char* toPath)
{
    std::error_code ec;
    std::filesystem::rename(makePath(fromPath), makePath(toPath), ec);
    return bool(ec);
}

bool
FS::mkdir(const char* path)
{
    return std::filesystem::create_directory(std::filesystem::path(path));
}

bool
FS::rmdir(const char* path)
{
    return remove(path);
}

std::filesystem::path
FS::makePath(const char* path)
{
    // All paths must be absolute. If the incoming path starts with '/'
    // prepend _cwd to it. Otherwise prepend with _cwd, then '/'.
    if (path == nullptr || path[0] == '\0') {
        // Empty path. just use _cwd
        return _rootDir;
    }
    
    auto p = _rootDir;
    p += path;
    p = p.lexically_normal();
    return p;
}

#endif
