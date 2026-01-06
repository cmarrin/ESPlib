/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "IDFWiFiPortal.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <wifi_provisioning/manager.h>

using namespace mil;

void
IDFWiFiPortal::begin(WebFileSystem*)
{
    // Setup prefs
    // _prefs.begin("ESPLib");

    // Initialize NVS partition
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Setup provisioning
    
}

void
IDFWiFiPortal::resetSettings()
{
}

void
IDFWiFiPortal::setTitle(const char* title)
{
}

void
IDFWiFiPortal::setMenu(std::vector<const char*>& menu)
{
}

void
IDFWiFiPortal::setDarkMode(bool b)
{
}

void
IDFWiFiPortal::setCustomMenuHTML(const char* html)
{
}

void
IDFWiFiPortal::setHostname(const char* name)
{
}

void
IDFWiFiPortal::setConfigHandler(HandlerCB f)
{
}

void
IDFWiFiPortal::setShowInfoErase(bool enabled)
{
}

int32_t
IDFWiFiPortal::addHTTPHandler(const char* endpoint, HandlerCB requestCB, HandlerCB uploadCB)
{
    return 0;
}

void
IDFWiFiPortal::serveStatic(const char *uri, const char *path)
{
}

bool
IDFWiFiPortal::autoConnect(char const *apName, char const *apPassword)
{
    return false;
}

void
IDFWiFiPortal::process()
{
}

void
IDFWiFiPortal::startWebPortal()
{
}

std::string
IDFWiFiPortal::localIP()
{
    return "0.0.0.0";
}

const char*
IDFWiFiPortal::getSSID()
{
    return "";
}

void
IDFWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data)
{
}

void
IDFWiFiPortal::sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip)
{
}    

void
IDFWiFiPortal::streamHTTPResponse(fs::File& file, const char* mimetype, bool attach)
{
}

int
IDFWiFiPortal::readHTTPContent(uint8_t* buf, size_t bufSize)
{
    return -1;
}

WiFiPortal::HTTPUploadStatus
IDFWiFiPortal::httpUploadStatus() const
{
    return HTTPUploadStatus::None;
}

std::string
IDFWiFiPortal::httpUploadFilename() const
{
    return "";
}

std::string
IDFWiFiPortal::httpUploadName() const
{
    return "";
}

std::string
IDFWiFiPortal::httpUploadType() const
{
    return "";
}

size_t
IDFWiFiPortal::httpUploadTotalSize() const
{
    return 0;
}

size_t
IDFWiFiPortal::httpUploadCurrentSize() const
{
    return 0;
}

const uint8_t*
IDFWiFiPortal::httpUploadBuffer() const
{
    return nullptr;
}

std::string
IDFWiFiPortal::getHTTPArg(const char* name)
{
    return "";
}

bool
IDFWiFiPortal::addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength)
{
    return false;
}

const char*
IDFWiFiPortal::getParamValue(const char* id)
{
    return nullptr;
}

#endif
