/*-------------------------------------------------------------------------
    This source file is a part of ESPlib
    For the latest info, see https://github.com/cmarrin/ESPlib
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

/*
    Vesper Controller
    
    This is a thin wrapper around the Application layer of ESPlib
*/

#include "Vesper.h"

#include "ESPWiFiPortal.h"

mil::ESPWiFiPortal portal;
Vesper controller(&portal);

void setup()
{
    Serial.begin(115200);
    delay(1000);
    controller.setup();
}

void loop()
{
    controller.loop();
}
