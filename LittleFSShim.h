/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "mil.h"

#include <fstream>
#include <filesystem>

// File System classes for Mac and esp-idf that duplicates the functionality 
// of the LittleFS filesystem for Arduino ESP32

static constexpr const char* FSPrefix = "fs";

enum SeekMode {
    SeekSet = 0,
    SeekCur = 1,
    SeekEnd = 2
};

class File
{
  public:
    File(const std::filesystem::path& path, const char* mode = "r");

    ~File() { close(); }
    
    size_t write(uint8_t);
    size_t write(const uint8_t* buf, size_t size);
    int read();
    int peek();
    void flush();
    size_t read(uint8_t* buf, size_t size);

    bool seek(uint32_t pos, SeekMode mode = SeekSet);
    size_t position() const;
    size_t size() const;
    
    void close();
    
    const char* path() const;
    const char* name() const;
    
    bool isDirectory();
    String getNextFileName();
    String getNextFileName(bool* isDir);
    void rewindDirectory();
    
    operator bool() const { return _good; }
  private:
    std::fstream _file;
    std::filesystem::directory_iterator _dir;
    bool _isDir = false;
    std::filesystem::path _path;
    bool _good = false;
};
    
class FS
{
  public:
    FS()
    {
        _rootDir = std::filesystem::current_path() / FSPrefix / "";
        if (!exists("")) {
            mkdir(_rootDir.c_str());
        }
    }
 
	bool begin(bool format) { return true; }
 
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
