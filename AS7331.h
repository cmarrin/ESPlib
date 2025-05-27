/*-------------------------------------------------------------------------
    This source file is a part of ESPLib
    For the latest info, see https://github.com/cmarrin/ESPLib
    Copyright (c) 2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

static constexpr DefaultI2CAddr = 0x74

// This library is the interface to the AS7331 UV Sensor. It uses I2C address
// 0x74 by default

namespace mil {

class AS7331
{
public:
	AS7331();

};

}
