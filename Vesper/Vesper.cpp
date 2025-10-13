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
    : mil::Application(portal, LED_BUILTIN, ConfigPortalName)
{
}

void
Vesper::setup()
{
    delay(500);
    Application::setup();

    setTitle("<center>MarrinTech Vesper Controller v0.1</center>");

    printf("Vesper Controller v0.1\n");
}
	
void
Vesper::loop()
{
    Application::loop();
}
