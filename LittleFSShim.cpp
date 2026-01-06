/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "LittleFSShim.h"

#if defined ESP_PLATFORM
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <sys/stat.h>
#include <unistd.h>
#include "esp_idf_version.h"
#include "esp_flash.h"
#include "esp_chip_info.h"
#include "spi_flash_mmap.h"

#include "esp_littlefs.h"
#else
#include <ftw.h>
#endif

using namespace fs;

File::File(const std::filesystem::path& path, const char* mode)
{
    close();
    
    _path = path;
    _name = _path.filename();

    if (std::filesystem::is_directory(_path)) {
        _isDir = true;
        _dir = std::filesystem::directory_iterator(_path);
    } else {
        _isDir = false;
        _file = fopen(_path.c_str(), mode);

        // errno gets set by std::filesystem::is_directory, so clear it here
        _error = (_file != nullptr) ? 0 : errno;
        errno = 0;
    }
}

int
File::write(const uint8_t* buf, size_t size)
{
    if (_isDir) {
        return -1;
    }
    
    int r = int(fwrite(buf, 1, size, _file));
    _error = errno;
    return r;
}

int
File::write(uint8_t c)
{
    return write(&c, 1);
}

int
File::read(uint8_t* buf, size_t size)
{
    if (_isDir) {
        return -1;
    }
    
    int r = int(fread(buf, 1, size, _file));
    _error = errno;
    return r;
}

int
File::read()
{
    if (_isDir) {
        return -1;
    }
    
    int c = fgetc(_file);
    _error = errno;
    return c;
}

int
File::peek()
{
    if (_isDir) {
        return -1;
    }
    
    int c = read();
    _error = errno;
    ungetc(c, _file);
    return c;
}

void
File::flush()
{
    if (_isDir) {
        return;
    }
    
    fflush(_file);
    _error = errno;
}

bool
File::seek(uint32_t pos, SeekMode mode)
{
    if (_isDir) {
        return -1;
    }
    
    int dir;
    switch(mode) {
        default     : dir = SEEK_SET; break;
        case SeekCur: dir = SEEK_CUR; break;
        case SeekEnd: dir = SEEK_SET; break;
    }
    bool b = fseek(_file, pos, dir) == 0;
    _error = errno;
    return b;
}

size_t
File::position()
{
    if (_isDir) {
        return -1;
    }
    
    size_t pos = ftell(_file);
    _error = errno;
    return pos;
}

size_t
File::size() const
{
    if (_isDir) {
        return 0;
    }
    
    return std::filesystem::file_size(path());
}

void
File::close()
{
    if (_file) {
        fclose(_file);
        _file = nullptr;
        _error = 0;
    }
}

const char*
File::path() const
{
    return _path.c_str();
}

const char*
File::name() const
{
    return _name.c_str();
}

bool
File::isDirectory()
{
    return _isDir;
}

File
File::openNextFile()
{
    if (!_isDir || _dir == std::filesystem::end(_dir)) {
        return File();
    }
    
    std::filesystem::path path = _dir->path();
    _dir++;
    return File(path);
}

void
File::rewindDirectory()
{
    if (!_isDir) {
        return;
    }
    _dir = std::filesystem::directory_iterator(_path);
}

bool
FS::begin(bool format)
{
    return false;
#if defined ESP_PLATFORM
    static const char *TAG = "LittleFSShim";

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "littlefs",
        .partition = nullptr,
        .format_if_mount_failed = format,
        .read_only = false,
        .dont_mount = false,
        .grow_on_mount = false,
    };
    
    esp_err_t ret = esp_vfs_littlefs_register(&conf);
    
    switch (ret) {
        case ESP_OK: break;
        case ESP_FAIL: ESP_LOGE(TAG, "Failed to mount or format filesystem\n"); return false;
        default: ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)\n", esp_err_to_name(ret)); return false;
    }

    uint32_t flashSize;
    esp_flash_get_size(nullptr, &flashSize);
    ESP_LOGI(TAG, "Flash size = %u\n", (unsigned int) flashSize);
    
    size_t total = 0;
    size_t used = 0;
    ret = esp_littlefs_info("littlefs", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "***** Failed to get LittleFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "LittleFS partition size: total: %u, used: %u", (unsigned int) total, (unsigned int) used);
    }
    
#endif
    return true;
}

size_t
FS::totalBytes()
{
#if defined ESP_PLATFORM
    size_t total = 0, used = 0;
    esp_err_t ret = esp_littlefs_info("littlefs", &total, &used);
    if (ret != ESP_OK) {
            ESP_LOGE("ESPlib_littlefs", "Failed to get LittleFS total bytes (%s)", esp_err_to_name(ret));
    }
    return total;
#else
    return TotalBytes;
#endif
}

size_t
FS::usedBytes()
{
#if defined ESP_PLATFORM
    size_t total = 0, used = 0;
    esp_err_t ret = esp_littlefs_info("littlefs", &total, &used);
    if (ret != ESP_OK) {
            ESP_LOGE("ESPlib_littlefs", "Failed to get LittleFS used bytes (%s)", esp_err_to_name(ret));
    }
    return used;
#else
    static size_t total;
    
    total = 0;
    if (ftw(_rootDir.c_str(),
        [](const char *path, const struct stat* sb, int flag) -> int
        {
            total += sb->st_size;
            return 0;
        }, 1)) {
        perror("ftw");
        return 2;
    }

    return total;
#endif
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
    return std::filesystem::create_directory(makePath(path));
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
