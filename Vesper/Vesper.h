/*-------------------------------------------------------------------------
    This source file is a part of ESPlib
    For the latest info, see https://github.com/cmarrin/ESPlib
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Application.h"

static constexpr const char* ConfigPortalName = "MT Vesper";
static constexpr int LEDPin = 10;

class Vesper : public mil::Application
{
  public:
    Vesper(mil::WiFiPortal*);

    virtual void setup() override;
    virtual void loop() override;
};

