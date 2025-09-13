/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

#include <map>
#include <memory>

// WiFiPortal is a generic class for handling connecting to WiFi. If
// there are saved WiFi credentials an attempt will be made to connect
// to that network. If that fails a captive portal will be created with
// a user supplied name and optional password. When connecting to the
// portal a collection of web pages will be shown with all scanned wifi 
// networks in the vicinity. The user can then enter credentials for the
// desired network, which are saved and then the system reboots and 
// attempts to connect to that network. The user can add custom fields
// to the portal's web pages, either to show custom information or to set 
// parameters specific to the application.
//
// When the network is connected the same web pages can be optionally
// shown, either at the IP address assigned or MDNS hostname, if one 
// exists. The hostname can be set on the same web pages as the WiFi 
// credentials.This is especially useful when the user adds parameters
// or info to the web pages.
//
// Showing the web pages implies that a web server is running. This 
// server can be used to handle GET or POST requests using custom
// handles. Any parameters or data sent with these requests is
// available to the user.
//
// Finally, the system supports a file system that can be read and
// written to store custom data for the application. This uses
// whatever filesystem is available, whether in flash, sd card or
// any other persistent storage.

class WiFiPortal
{
public:
    enum class HTTPMethod { Get, Post, Any };

    using HandlerCB = std::function<void(WiFiPortal*)>;
    using HandleRequestCB = std::function<bool(WiFiPortal*, HTTPMethod method, const String& uri)>;
    
    WiFiPortal() { }
    virtual ~WiFiPortal() { }
    
    virtual void begin() { }

    // Reset the saved SSID and password. On the next reboot or when autoConnect is called
    // the captive portal will be started
    virtual void resetSettings() { }
    
    // Set the title to be shown at the top of the front web page
    virtual void setTitle(String title) { }

    // This sets a list of info to be shown on the front page of the web portal, Some 
    // values show buttons to other pages, some show custom information. Currently
    // supported values:
    //
    //      "wifi"      - Show a button to the WiFi settings page
    //      "info"      - Show a button to a page with general system info
    //      "restart"   - Show a button which restarts the system
    //      "update"    - Show a button which allows an OTA update to be started
    //      "sep"       - Show a single line separator
    //      "custom"    - Show custom HTML, set with a call to setCustomMenuHTML
    //
    // Limitations:
    //      - Only one "custom" section is supported
    //      - All params set with addParam are shown on the Wifi settings page
    
    virtual void setMenu(std::vector<const char*>& menu) { }

    // Select the dark or light mode style sheet for the web pages
    virtual void setDarkMode(bool) { }
    
    // Get the value with the passed id saved to persistent storage
    virtual String getPrefString(const char* id) { return String(); }
    
    // Save the passed string at the passed id in persistent storage
    virtual void putPrefString(const char* id, const char* value) { }

    // Sets a string of static HTML that will be inserted into the front page at the
    // "custom" menu item
    virtual void setCustomMenuHTML(const char* html) { }

    // Set the hostname to use for MDNS
    virtual void setHostname(const char*) { }
    
    // Set the callback that will be called when the system enters the captive portal
    virtual void setConfigHandler(HandlerCB) { }

    // If enabled shows an "Erase WiFi Config" button on the info page
    // FIXME: very confusing that this is on the info page and not on the WiFi settings page
    virtual void setShowInfoErase(bool enabled) { }
    
    // Call the passed handler function when a request is made to the endpoint with the passed name.
    // Returns an id of the request for later use in deleting the request (not yet implemented).
    // The callback return true if it handled the request and false if not.
    virtual int32_t addHTTPHandler(const char* endpoint, HandleRequestCB) { return -1; }
    
    ///
    /// These next calls are used to either start the web portal or are called after
    /// it has started
    ///

    // Attempt to connect to a saved SSID and IP if successful, return true. Otherwise attempt
    // to start a captive portal at the passed SSID and password. If successful the method
    // next returns. If the user enters an SSID and password in the portal the system
    // reboots and attempts to connect.If this method returns false it means that starting
    // the captive portal was unsuccessfil.
    //
    // FIXME: We need to call MDNS.begin(hostname) when connection to the network is successful
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) { return true; }

    // This method is called on every time through the loop if the web server is running in
    // the main thread (e.g., Arduino). Otherwise this method is empty.
    virtual void process() { }
    
    // Manually start the web portal. This is done after successful connection to
    // the network to make the web pages available at run time.
    virtual void startWebPortal() { }
    
    // Return the IP address. In config mode, this is the soft AP IP address.
    virtual String localIP() { return String(); }

    // Get the SSID of the captive portal if in config mode. Otherwise get the SSID of the network currently connected to
    virtual const char* getSSID() { return "unknown"; }
    
    // Send a response to the requestor
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const String& data = String()) { }
    
    // Read content from an incoming HTTP request. Must be called from inside a HandleRequestCB. A buffer no larger
    // than bufSize will be returned, but it could be smaller. If data was returned the return value will
    // be the positive number of bytes returned. A value of 0 mean there is no more data to return and a 
    // negative value means there was an error.
    virtual int readHTTPContent(uint8_t* buf, size_t bufSize) { return -1; }
    
    // Get the content-length passed in the request. Must be called inside a HandleRequestCB
    virtual size_t httpContentLength() { return 0; }

    // Extract the value for the passed name from the passed uri. Arguments start after the first '?' and are of the form
    // <name>=<value>. Args are separated with '&'.
    static String getHTTPArg(const String& uri, const char* name)
    {
        int16_t start = uri.indexOf('?');
        if (start < 0) {
            return String();
        }
        
        // TODO: Implement
        return String();
    }
    
    // Param handling
    //
    // These are generic and store values in a runtime list. They are backed by the 
    // persistent storage prefs system which is specialized for each platform. At the
    // start addParam is called for each param to be saved. If there is already a
    // prefs value saved it is used in place of the passed defaultValue.
    //
    // Params show on the WiFi config page as settable fields with the passed label and 
    // a text field with maxLength.
    bool addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength)
    {
        // See if we have a duplicate
        if (_params.contains(id)) {
            printf("***** Error: addParam, id '%s' already exists\n", id);
           return false;
        }
        
        // Save param in prefs
        String savedValue = getPrefString(id);
        if (savedValue.length() == 0) {
            putPrefString(id, defaultValue);
            printf("No '%s' saved. Setting it to default: '%s'\n", id, defaultValue);
        } else {
            defaultValue = savedValue.c_str();
            printf("Setting '%s' to saved value: '%s'\n", id, savedValue.c_str());
        }

        _params.insert({ id, { label, defaultValue, maxLength } });
        return true;
    }

    const char* getParamValue(const char* id) const
    {
        const auto& p = _params.find(id);
        return p == _params.end() ? nullptr : p->second.value.c_str();
    }
    
protected:
    struct Param
    {
        String label;
        String value;
        uint32_t length;
    };

    std::map<String, Param> _params;
};
