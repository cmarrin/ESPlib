/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

#include <fstream>
#include <filesystem>

// File System classes for Mac and esp-idf that duplicates the functionality 
// of the LittleFS filesystem for Arduino ESP32

namespace fs {

enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

class File
{
  public:
    File(const std::filesystem::path& path, const char* mode = "r");
    File() { }
    File(const File&) = delete;
    File(File&& other) { *this = std::move(other); }

    ~File() { close(); }
    
    File& operator=(const File&) = delete;
    
    File& operator=(File&& other)
    {
        _dir = std::move(other._dir);
        _isDir = other._isDir;
        _path = std::move(other._path);
        _name = std::move(other._name);
        _error = other._error;
        _file = other._file;
        other._file = nullptr;
        return *this;
    }
    
    int write(uint8_t);
    int write(const uint8_t* buf, size_t size);
    int read();
    int peek();
    void flush();
    int read(uint8_t* buf, size_t size);

    bool seek(uint32_t pos, SeekMode mode = SeekSet);
    size_t position();
    size_t size() const;
    
    void close();
    
    const char* path() const;
    const char* name() const;
    
    bool isDirectory();
    File openNextFile();
    void rewindDirectory();
    
    operator bool() const { return _isDir || (_file && _error == 0); }
    
  private:
    std::filesystem::directory_iterator _dir;
    bool _isDir = false;
    std::filesystem::path _path;
    std::string _name; // We need to keep this so we can return it as a const char*
    int _error = 0;
    FILE* _file = nullptr;
};
    
class FS
{
  public:
    // Similated max size is around 4MB
    static constexpr size_t TotalBytes = 1 * 1024 * 1024; 
    FS()
    {
#if defined ESP_PLATFORM
        _rootDir = "/littlefs/";
#else
        _rootDir = std::filesystem::current_path() / "littlefs" / "";
        if (!exists("")) {
            mkdir(_rootDir.c_str());
        }
#endif
    }
 
	bool begin(bool format);
 
    size_t totalBytes();
    size_t usedBytes();

    File open(const char* path, const char* mode = "r", bool create = false);
    
    bool exists(const char* path);
    bool remove(const char* path);
    bool rename(const char* fromPath, const char* toPath);
    bool mkdir(const char* path);
    bool rmdir(const char* path);
    
    // Create an absolute path starting with _cwd
    std::filesystem::path makePath(const char* path);

  private:
    std::filesystem::path _rootDir;
};

}
