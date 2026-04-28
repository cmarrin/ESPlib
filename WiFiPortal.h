/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#pragma once

#include "System.h"

#include <map>

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

    using HandlerCB = std::function<void(WiFiPortal*, const char* tail)>;

    struct KnownNetwork
    {
        bool operator==(const KnownNetwork& other) const { return ssid == other.ssid; }
        bool operator<(const KnownNetwork& other) const { return ssid < other.ssid; }
        std::string ssid; 
        int8_t rssi; 
        bool open;
    };

    WiFiPortal() { }
    virtual ~WiFiPortal() { }
    
    virtual void begin(WebFileSystem*) { }

    // Set the callback that will be called when the system enters the captive portal
    virtual void setConfigHandler(HandlerCB) { }

    // Call the passed handler function when a request is made to the endpoint with the passed name.
    // Returns an id of the request for later use in deleting the request (not yet implemented).
    // The callback return true if it handled the request and false if not.
    virtual int32_t addHTTPHandler(const char* endpoint, HTTPMethod, HandlerCB requestCB) { return -1; }
    
    // Serve static pages. When an endpoint starting in uri is seen it responds with the file at
    // the passed path as its root.
    virtual void addStaticHTTPHandler(const char *uri, const char *path) { }

    ///
    /// These next calls are used to either start the web portal or are called after
    /// it has started
    ///

    // Attempt to connect to a saved SSID and IP if successful, return true. Otherwise attempt
    // to start a captive portal at the passed SSID and password. If successful the method
    // next returns. If the user enters an SSID and password in the portal the system
    // reboots and attempts to connect.If this method returns false it means that starting
    // the captive portal was unsuccessfil.
    virtual bool autoConnect(char const *apName, char const *apPassword = NULL) { return true; }

    // This method is called on every time through the loop if the web server is running in
    // the main thread (e.g., Arduino). Otherwise this method is empty.
    virtual void process() { }
    
    // Return the IP address. In config mode, this is the soft AP IP address.
    virtual std::string getIP() { return ""; }

    // Get the SSID of the captive portal if in config mode. Otherwise get the SSID of the network currently connected to
    virtual const char* getSSID() { return "unknown"; }
    
    // Send a response to the requestor. The second form allows sending of binary data, possibly in GZIP format
    virtual void sendHTTPResponse(int code, const char* mimetype = nullptr, const char* data = "") { }
    virtual void sendHTTPResponse(int code, const char* mimetype, const char* data, size_t length, bool gzip) { }
    
    // Send a response with the contents of the passed File
    virtual void streamHTTPResponse(fs::File& file, const char* mimetype, bool attach) { }
    
    // These methods get values for the current upload. Must be called inside a HandlerCB
    virtual HTTPUploadStatus httpUploadStatus() const { return HTTPUploadStatus::None; }
    virtual std::string httpUploadFilename() const { return ""; }
    virtual size_t httpUploadTotalSize() const { return 0; }
    virtual size_t httpUploadCurrentSize() const { return 0; }
    virtual const uint8_t* httpUploadBuffer() const { return nullptr; }
    
    // This method receives data from an open response. It's used for non-multipart POST
    virtual int receiveHTTPResponse(char* buf, size_t size) { return 0; }

    // Extract the value for the passed name from the passed uri. Arguments start after the first '?' and are of the form
    // <name>=<value>. Args are separated with '&'.
    virtual std::string getHTTPArg(const char* name) { return ""; }
    
    // Add values to the arg list, for instance from a form-data response
    virtual void parseQuery(const char* queryString) { }

    virtual std::string getHTTPHeader(const char* name) { return ""; }

    virtual void otaUpdate() { }

    // Get Info
    virtual std::string getCPUModel() const { return ""; }
    virtual uint32_t getCPUFrequency() const { return 50; }
    virtual float getCPUTemperature() const { return 25.0; }
    virtual uint32_t getCPUUptime() const { return 0; }

    // Get/Set/Erase params in non-volatile storage
    virtual void setNVSParam(const char* id, const std::string& value) { }
    virtual bool getNVSParam(const char* id, std::string& value) const { return false; }
    virtual void eraseNVSParam(const char* id) { }
    
    // Set the title to be shown at the top of the front web page
    void setTitle(const char* title) { _title = title; }
    const std::string& getTitle() const { return _title; }

    // Set the hostname to use for MDNS
    void setHostname(const char* hostname) { _hostname = hostname; }
    const std::string& getHostname() const { return _hostname; }
    
    // Sets a string of static HTML that will be inserted into the front page at the
    // "custom" menu item
    void setCustomInfoHandler(std::function<std::string()> cb) { _customInfoHTMLHandler = cb; }
    const std::string getCustomInfoHTML() const { return _customInfoHTMLHandler ? _customInfoHTMLHandler() : ""; }
    const std::string getCustomTextForms() const
    {
        std::string forms = "[ ";
        bool first = true;
        for (const auto& it : _paramMap) {
            if (!first) {
                forms += ", ";
            } else {
                first = false;
            }
            
            // Get current value
            std::string value;
            getNVSParam(it.first.c_str(), value);
        
            forms += "{ \"id\" : \"" + it.first + 
                     "\", \"label\" : \"" + it.second.label + 
                     "\", \"maxLength\" : \"" + std::to_string(it.second.maxLength) + 
                     "\", \"value\" : \"" + value + 
                     "\", \"defaultValue\" : \"" + it.second.defaultValue + 
                     "\" }";
        }
        forms += " ]";
        return forms;
    }

    const std::string& getGateway() const { return _currentGW; }
    const std::string& getMask() const { return _currentMSK; }
    const std::string& getDNS() const { return _currentDNS; }

    bool isProvisioning() const { return _provisioning; }
    
    const std::vector<KnownNetwork>* getKnownNetworks() const { return &_knownNetworks; }

  private:
    // Param Map
    struct ParamMapValue { std::string label; uint32_t maxLength; std::string defaultValue; };
    std::map<std::string, ParamMapValue> _paramMap;

  public:
    void addFormEntry(const char *id, const char* label, const char* defaultValue, uint32_t maxLength)
    {
        // First we have to see if there is a saved value for this id. If so use it in place of the defaultValue
        std::string value;

        if (!getNVSParam(id, value) || value.empty()) {
            value = defaultValue;
            System::logI("WiFiPortal", "No '%s' saved. Setting it to default: '%s'\n", id, defaultValue);
        } else {
            System::logI("WiFiPortal", "Setting '%s' to saved value: '%s'\n", id, value.c_str());
        }
        setNVSParam(id, value);

        // Now fill in the param map. This will save the default value and max length of the type-in field
        auto entry = _paramMap.find(id);
        if (entry == _paramMap.end()) {
            // New map entry
            _paramMap.insert({ id, { label, maxLength, defaultValue } });
        }
    }
    
    bool getFormEntryId(uint32_t i, std::string& value)
    {
        if (i >= _paramMap.size()) {
            return false;
        }
        for (const auto& it : _paramMap) {
            if (i == 0) {
                value = it.first;
                return true;
            }
            --i;
        }
        return false;
    }
    
  protected:
    std::string _ssid;
    std::string _pass;
    std::string _hostname;
    std::string _currentGW = "0.0.0.0";
    std::string _currentMSK = "255.255.255.255";
    std::string _currentDNS = "0.0.0.0";
    
    bool _provisioning = false;
    std::vector<KnownNetwork> _knownNetworks;

  private:
    std::string _title;
    std::function<std::string()> _customInfoHTMLHandler;
};

}
