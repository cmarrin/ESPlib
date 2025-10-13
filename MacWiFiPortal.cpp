/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "MacWiFiPortal.h"

void
MacWiFiPortal::begin()
{
}

void
MacWiFiPortal::resetSettings()
{
}

void
MacWiFiPortal::setTitle(const char* title)
{
}

void
MacWiFiPortal::setMenu(std::vector<const char*>& menu)
{
}

void
MacWiFiPortal::setDarkMode(bool)
{
}

void
MacWiFiPortal::setCustomMenuHTML(const char* html)
{
}

void
MacWiFiPortal::setHostname(const char*)
{
}

void
MacWiFiPortal::setConfigHandler(HandlerCB)
{
}

void
MacWiFiPortal::setShowInfoErase(bool enabled)
{
}

int32_t
MacWiFiPortal::addHTTPHandler(const char* endpoint, HandlerCB)
{
    return -1;
}

bool
MacWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    return true;
}

void
MacWiFiPortal::process()
{
}

void
MacWiFiPortal::startWebPortal()
{
}

std::string
MacWiFiPortal::localIP()
{
    return "";
}

const char*
MacWiFiPortal::getSSID()
{
    return "";
}

void
MacWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const std::string& data)
{
}

int
MacWiFiPortal::readHTTPContent(uint8_t* buf, size_t bufSize)
{
    return -1;
}

size_t
MacWiFiPortal::httpContentLength()
{
    return 0;
}

#endif
