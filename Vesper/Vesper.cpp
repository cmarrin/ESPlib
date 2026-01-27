/*-------------------------------------------------------------------------
    This source file is a part of PostLightController
    For the latest info, see https://github.com/cmarrin/PostLightController
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Vesper.h"

Vesper::Vesper(mil::WiFiPortal* portal)
    : mil::Application(portal, ConfigPortalName)
{
}

void
Vesper::setup()
{
    delay(500);
    Application::setup();

    setTitle((std::string("<center>MarrinTech Vesper Controller v") + Version + "</center>").c_str());

    printf("Vesper Controller v%s\n", Version);
}
	
void
Vesper::loop()
{
    Application::loop();
}
