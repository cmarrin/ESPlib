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
    File(const File& file)
        : _file(file._file)
        , _dir(file._dir)
        , _isDir(file._isDir)
        , _path(file._path)
        , _name(file._name)
    {
    }

    ~File() { close(); }
    
    int write(uint8_t);
    int write(const uint8_t* buf, size_t size);
    int read();
    int peek();
    void flush();
    int read(uint8_t* buf, size_t size);

    bool seek(uint32_t pos, SeekMode mode = SeekSet);
    size_t position() const;
    size_t size() const;
    
    void close();
    
    const char* path() const;
    const char* name() const;
    
    bool isDirectory();
    File openNextFile();
    void rewindDirectory();
    
    operator bool() const { return _isDir || (_file && !_file->fail()); }
    
  private:
    std::fstream* _file = nullptr;
    std::filesystem::directory_iterator _dir;
    bool _isDir = false;
    std::filesystem::path _path;
    std::string _name; // We need to keep this so we can return it as a const char*
};
    
class FS
{
  public:
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
    
  private:
    // Create an absolute path starting with _cwd
    std::filesystem::path makePath(const char* path);
    
    std::filesystem::path _rootDir;
};
