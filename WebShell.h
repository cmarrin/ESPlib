/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

// Web based filesystem. It can be integrated with WebServer
// so HTTP requests can be made to upload files, read existing
// files, create directories, delete files and directories and do
// directory listings.

namespace mil {

class Application;

class WebShell
{
  public:    
    bool begin(Application*);
    
  private:
};

}
