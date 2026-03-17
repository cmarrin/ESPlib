/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

#include <filesystem>

// File System classes for Mac and esp-idf that duplicates the functionality 
// of the LittleFS filesystem for Arduino ESP32

enum class SeekMode { Set = 0, Cur = 1, End = 2 };

class Dir
{
  public:
    Dir() { }
    Dir(const char* path);
    bool next();
    const char* fileName() const;
    size_t fileSize() const;
    bool isDirectory() const;
    
  private:
    std::filesystem::directory_iterator _dir;
    mutable std::string _name;
    bool _open = false;
    
    // LittleFS starts it dir object not pointing at the first file,
    // but directory_iterator does. So we need to handle the first
    // call to next() specially
    bool first = true;
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
        close();
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
    bool flush();
    int read(uint8_t* buf, size_t size);

    bool seek(uint32_t pos, SeekMode mode = SeekMode::Set);
    size_t position();
    size_t size() const;
    
    bool close();
    
    const char* fileName() const;
    size_t fileSize() const { return std::filesystem::file_size(_path); }
    
    bool isDirectory() const;
    bool next() { return false; }
    
    operator bool() const { return _isDir || (_file && _error == 0); }

    int error() const { return _error; }
    
    void clearError()
    {
        _error = 0;
        if (_file) {
            clearerr(_file);
        }
    }
    
  private:
    std::filesystem::path _path;
    std::string _name; // We need to keep this so we can return it as a const char*
    int _error = 0;
    FILE* _file = nullptr;
    bool _isDir = false;
};

namespace fs {

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
        _rootDir = (std::filesystem::current_path() / "littlefs" / "").string();
        if (!exists(_rootDir.c_str())) {
            mkdir(_rootDir.c_str());
        }
#endif
    }
 
	bool begin(bool format);
 
    size_t totalBytes();
    size_t usedBytes();

    File open(const char* path, const char* mode = "r", bool create = false);
    Dir openDir(const char* path);
    
    bool exists(const char* path);
    bool remove(const char* path);
    bool rename(const char* fromPath, const char* toPath);
    bool mkdir(const char* path);
    bool rmdir(const char* path);

    std::string rootDir() const { return _rootDir; }
    
  private:
    std::string _rootDir;
};

}

extern fs::FS LittleFS;
