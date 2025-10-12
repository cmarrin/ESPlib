/*-------------------------------------------------------------------------
This source file is a part of mil

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#include "WebShell.h"

#include "Application.h"

using namespace mil;

bool
WebShell::begin(Application* app)
{
    app->addHTTPHandler("/shell", [this](WiFiPortal* p)
    {
        p->sendHTTPResponse(404, "text/plain", "Not yet impolemented");
        return true;
    });

    return true;
}
