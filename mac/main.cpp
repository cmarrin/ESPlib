/*-------------------------------------------------------------------------
    This source file is a part of ESPlib
    For the latest info, see https://github.com/cmarrin/ESPlib
    Copyright (c) 2021-2022, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Vesper.h"

#include "MacWiFiPortal.h"

mil::MacWiFiPortal portal;

int main(int argc, char * const argv[])
{
    printf("Vesper Simulator\n\n");

    Vesper controller(&portal);
    
    controller.setup();
    
    while (true) {
        controller.loop();
    }
                        
    return 1;
}
