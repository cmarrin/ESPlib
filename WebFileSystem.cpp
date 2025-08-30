/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebFileSystem.h"

using namespace mil;

#ifndef ARDUINO
FS LittleFS;
#endif

bool
WFS::begin(bool format)
{
    return LittleFS.begin(format);
}
    
File
WFS::open(const char* path, const char* mode, bool create)
{
    return LittleFS.open(path, mode);
}
