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
    _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this));
}

void
Vesper::setup()
{
    delay(500);
    Application::setup();

    setTitle((std::string("<center>MarrinTech Vesper Controller v") + Version + "</center>").c_str());

    printf("Vesper Controller v%s\n", Version);
    
    setCustomMenuHandler([this]()
    {
        std::string s;
        s = "<span style='font-size:small;margin-top:0px;'><strong>Time/Date:</strong> ";
        s += _clock->prettyTime();
        s += " <span style='font-size:x-small'>(at last refresh)</span></span>";
        s += "<br><span style='font-size:small'><strong>Weather:</strong> ";
        s += _clock->weatherConditions();
        s += "  Cur:";
        s += std::to_string(_clock->currentTemp());
        s += "°  Hi:";
        s += std::to_string(_clock->highTemp());
        s += "°  Lo:";
        s += std::to_string(_clock->lowTemp());
        s += "°</span><br><br>";
        return s;
    });
    
    _clock->setup();
}
	
void
Vesper::loop()
{
    Application::loop();
    _clock->loop();
}
