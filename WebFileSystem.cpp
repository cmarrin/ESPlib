/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebFileSystem.h"

#include "LittleFS.h"

using namespace mil;

bool
File::write(const uint8_t* buf, uint32_t size)
{
}

bool
File::write(uint8_t)
{
}

bool
File::read(uint8_t* buf, uint32_t size)
{
}

int32_t
File::read()
{
}

int32_t
File::peek()
{
}

void
File::flush()
{
}

bool
File::seek(uint32_t pos, Seek mode = Seek::Set)
{
}

uint32_t
File::tell() const
{
}

uint32_t
File::size() const
{
}

void
File::close()
{
}

const char*
File::path() const
{
}

const char*
File::name() const
{
}

bool
File::isDirectory()
{
}

String
File::getNextFileName()
{
}

String
File::getNextFileName(bool& isDir)
{
}

void
File::rewindDirectory()
{
}
