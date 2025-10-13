/*-------------------------------------------------------------------------
    This source file is a part of ESPlib
    For the latest info, see https://github.com/cmarrin/ESPlib
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Vesper.h"

#include "WiFiPortal.h"

int main(int argc, char * const argv[])
{
    printf("Vesper Simulator\n\n");

    mil::WiFiPortal portal;
    Vesper controller(&portal);
    
    controller.setup();
    
    while (true) {
        controller.loop();
    }
                        
    return 1;
}
