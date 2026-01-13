/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

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

namespace fs {
    class File;
}

namespace mil {

class WebFileSystem;

class WiFiPortal
{
public:
    enum class HTTPUploadStatus { None, Start, Write, End, Aborted };
    enum class HTTPMethod { Get, Post, Put };

    using HandlerCB = std::function<void(WiFiPortal*)>;
    
    WiFiPortal() { }
    virtual ~WiFiPortal() { }
    
    virtual void begin(WebFileSystem*) { }

    // Reset the saved SSID and password. On the next reboot or when autoConnect is called
    // the captive portal will be started
    virtual void resetSettings() { }
    
    // Set the title to be shown at the top of the front web page
    virtual void setTitle(const char* title) { }

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
    virtual int32_t addHTTPHandler(const char* endpoint, HTTPMethod, HandlerCB requestCB, HandlerCB uploadCB) { return -1; }
    
    int32_t addHTTPHandler(const char* endpoint, HTTPMethod method, HandlerCB requestCB)
    {
        return addHTTPHandler(endpoint, method, requestCB, nullptr);
    }
    int32_t addHTTPHandler(const char* endpoint, HandlerCB requestCB, HandlerCB uploadCB)
    {
        return addHTTPHandler(endpoint, HTTPMethod::Get, requestCB, uploadCB);
    }
    int32_t addHTTPHandler(const char* endpoint, HandlerCB requestCB)
    {
        return addHTTPHandler(endpoint, HTTPMethod::Get, requestCB, nullptr);
    }
    
    // Serve static pages. When an endpoint starting in uri is seen it responds with the file at
    // the passed path as its root.
    virtual void serveStatic(const char *uri, const char *path) { }

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
    virtual std::string localIP() { return ""; }

    // Get the SSID of the captive portal if in config mode. Otherwise get the SSID of the network currently connected to
    virtual const char* getSSID() { return "unknown"; }
    
    // Send a response to the requestor. The second form allows sending of binary data, possibly in GZIP format
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "") { }
    virtual void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip) { }
    
    // Send a response with the contents of the passed File
    virtual void streamHTTPResponse(fs::File& file, const char* mimetype, bool attach) { }
    
    // Read content from an incoming HTTP request. Must be called from inside a HandlerCB. A buffer no larger
    // than bufSize will be returned, but it could be smaller. If data was returned the return value will
    // be the positive number of bytes returned. A value of 0 mean there is no more data to return and a 
    // negative value means there was an error.
    virtual int readHTTPContent(uint8_t* buf, size_t bufSize) { return -1; }
    
    // These methods get values for the current upload. Must be called inside a HandlerCB
    virtual HTTPUploadStatus httpUploadStatus() const { return HTTPUploadStatus::None; }
    virtual std::string httpUploadFilename() const { return ""; }
    virtual std::string httpUploadName() const { return ""; }
    virtual std::string httpUploadType() const { return ""; }
    virtual size_t httpUploadTotalSize() const { return 0; }
    virtual size_t httpUploadCurrentSize() const { return 0; }
    virtual const uint8_t* httpUploadBuffer() const { return nullptr; }

    // Extract the value for the passed name from the passed uri. Arguments start after the first '?' and are of the form
    // <name>=<value>. Args are separated with '&'.
    virtual std::string getHTTPArg(const char* name) { return ""; }
    
    // Param handling
    virtual bool addParam(const char *id, const char* label, const char* defaultValue, uint32_t maxLength) { return true; }
    virtual bool getParamValue(const char* id, std::string& value) { return false; }
};

}
