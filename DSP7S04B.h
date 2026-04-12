/*

Copyright (c) 2016, Embedded Adventures, www.embeddedadventures.com
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
 
- Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

- Neither the name of Embedded Adventures nor the names of its contributors
  may be used to endorse or promote products derived from this software
  without specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
THE POSSIBILITY OF SUCH DAMAGE.

Contact us at source [at] embeddedadventures.com

*/

//
// This code implements the interface for the Embedded Adventures 4 digit
// clock display over i2c. The interface is here:
//
//  https://www.embeddedadventures.com/datasheets/DSP-7S04B_hw_v3_doc_v1.pdf
//
// Display is written to in raw mode. That way we can write to a GraphicsCanvas1
// and then transfer to the display all at once. This allows us to simulate
// on the Mac

#pragma once

#include "Graphics.h"
#include "System.h"

namespace mil {

class DSP7S04B
{
public:
    DSP7S04B(RenderCB renderCB);
    
    void begin();
    
    void print(const char* str);
    void clearDisplay(void);
    void setDot(uint8_t pos, bool on);
    void setColon(bool on);

    // Platform specific
    void setBrightness(uint8_t level);
    void refresh();

private: 
    GraphicsCanvas1 _canvas;
    
    // Raw format. Each digit is 2 bytes.
    // Upper byte is DP, G, F, E, D, C, B, A
    // Lower byte is all 0, except the rightmost digit which has the colon in the LSB
    
    RenderCB _renderCB;
};

}
