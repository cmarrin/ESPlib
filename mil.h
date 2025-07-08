//
//  mil.h
//
//  Created by Chris Marrin on 3/19/2011.
//
//

/*
Copyright (c) 2009-2011 Chris Marrin (chris@marrin.com)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, this 
      list of conditions and the following disclaimer.

    - Redistributions in binary form must reproduce the above copyright notice, 
      this list of conditions and the following disclaimer in the documentation 
      and/or other materials provided with the distribution.

    - Neither the name of Marrinator nor the names of its contributors may be 
      used to endorse or promote products derived from this software without 
      specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN 
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE.
*/

#pragma once

/*  This is the minimum file to use AVR
    This file should be included by all files that will use 
    or implement the interfaces
*/

#ifdef ARDUINO
#include <Arduino.h>
#include <Printable.h>
#include <Ticker.h>

#define ToString(v) String(v)
#define ToFloat(s) s.toFloat()

#else
#include <cstdint>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>

#define cout std::cout
#define F(s) s
#define ToString(v) std::to_string(v)
#define ToFloat(s) std::stof(s)
#define PROGMEM

static inline uint32_t millis() { return uint32_t((double) clock() / CLOCKS_PER_SEC * 1000); }

class String : public std::string
{
  public:
    String() : std::string() { }
    String(const char* s) : std::string(s) { }
    
    bool startsWith(String s) const { return s.starts_with(s); }
    int16_t indexOf(char val, uint16_t from = 0) const
    {
        int16_t r = find(val, from);
        return (r == npos) ? -1 : r;
    }
    
    String toString() { return *this; }
};

static inline void delay(uint32_t ms) { useconds_t us = useconds_t(ms) * 1000; usleep(us); }
static inline void pinMode(uint8_t, uint8_t) { }
static inline uint32_t analogRead(uint8_t) { return 0; }
static inline bool digitalRead(uint8_t) { return 1; } // digital pins are active low

static constexpr uint8_t INPUT = 0;
static constexpr uint8_t INPUT_PULLUP = 0;
static constexpr uint8_t LED_BUILTIN = 0;

#define NEO_GRB 0
#define NEO_KHZ800 0

class DSP7S04B
{
public:
    void setDot(uint8_t pos, bool on) { if (on) { cout << "Dot set\n"; } }
    void setColon(bool on) { _colon = on; }
    void clearDisplay(void) { }
    void setBrightness(uint8_t b) { cout << "*** Brightness set to " << b << "\n"; }

    void print(const char* str)
    {
        if (_colon) {
            cout << str[0] << str[1] << ":" << str[2] << str[3] << "\n";
        } else {
            cout << str << "\n";
        }
    }
      
private: 
     bool _colon = false;
 
};

class Max7219Display
{
public:
    Max7219Display(std::function<void()> scrollDone) : _scrollDone(scrollDone) { }

    void clear() { }
    void setBrightness(uint32_t b) { cout << "*** Brightness set to " << b << "\n"; }
    
    void showString(const char* str, uint32_t underscoreStart = 0, uint32_t underscoreLength = 0)
    {
        // String might start with \a or \v. Skip them
        const char* s = str;
        if (s[0] == '\a' || s[0] == '\v') {
            s += 1;
        }
        cout << "\n[[ " << s << " ]]\n\n";
        
        // If we're scrolling we need to call _scrollDone
        if (str[0] == '\v') {
            _scrollDone();
        }
    }

private:
    std::function<void()> _scrollDone;

};

class Ticker
{
public:
    using callback_t = std::function<void(void)>;
    
    void once_ms(uint32_t ms, callback_t callback)
    {
        _ms = ms;
        _cb = callback;

        std::thread([this]() { 
            std::this_thread::sleep_for(std::chrono::milliseconds(_ms));
            _cb();
        }).detach();
    }
    
    void attach_ms(uint32_t ms, callback_t callback)
    {
        _ms = ms;
        _cb = callback;

        std::thread([this]() { 
            while (true) { 
                auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(_ms);
                _cb();
                std::this_thread::sleep_until(x);
            }
        }).detach();
    }

private:
    uint32_t _ms = 0;
    callback_t _cb;
};

class WiFiClass
{
public:
    const char* softAPIP() { return "127.0.0.1"; }
    String localIP() { return "127.0.0.1"; }
};

extern WiFiClass WiFi;

class MDNSClass
{
public:
    static bool begin(const char*) { return true; }
    static void update() { }
};

extern MDNSClass MDNS;

class Preferences
{
public:
    void begin(const char*) { }
    String getString(const char*) { return ""; }
    void putString(const char*, const char*) { }
};

class WiFiManagerParameter
{
public:
    WiFiManagerParameter(const char *id, const char *label, const char *defaultValue, int length)
        : _id(id)
        , _value(defaultValue)
        , _length(length)
    { }
    
    const char* getID() const { return _id.c_str(); }
    const char* getValue() const { return _value.c_str(); }
    void setValue(const char *defaultValue, int length) { _value = defaultValue; }
    int getValueLength() const { return _length; }

private:
    std::string _id;
    std::string _value;
    int _length;
};

enum HTTPRawStatus {
  RAW_START,
  RAW_WRITE,
  RAW_END,
  RAW_ABORTED
};

#define HTTP_UPLOAD_BUFLEN 1

typedef struct {
  HTTPRawStatus status;
  size_t totalSize;    // content size
  size_t currentSize;  // size of data currently in buf
  uint8_t buf[1];
  void *data;  // additional data
} HTTPRaw;

enum HTTPMethod { HTTP_GET, HTTP_POST };

enum HTTPUploadStatus {
  UPLOAD_FILE_START,
  UPLOAD_FILE_WRITE,
  UPLOAD_FILE_END,
  UPLOAD_FILE_ABORTED
};

typedef struct {
  HTTPUploadStatus status;
  String filename;
  String name;
  String type;
  size_t totalSize;    // file size
  size_t currentSize;  // size of data currently in buf
  uint8_t buf[HTTP_UPLOAD_BUFLEN];
} HTTPUpload;

class RequestHandler;

class WebServer
{
  public:
    void on(const char* page, std::function<void(void)> handler) { }
    void on(const char* page, int method, std::function<void(void)> h, std::function<void(void)> uh) { }
    String arg(const char*) { return ""; }
    int args() { return 0; }
    void send(int, const char* = nullptr, const String& = String()) { }
    const HTTPRaw& raw() const { return _raw; }
    void addHandler(RequestHandler *handler) { }
    HTTPRaw _raw;
};

class RequestHandler {
  public:
    virtual bool canHandle(WebServer& server, HTTPMethod method, const String& uri) { return false; }
    virtual bool canUpload(WebServer& server, const String& uri) { return false; }
    virtual bool handle(WebServer& server, HTTPMethod method, const String& uri) { return false; }
    virtual void upload(WebServer& server, const String &uri, HTTPUpload &upload) { }
};

class WiFiManager
{
public:
    void setDebugOutput(bool) { }
    void resetSettings() { }
    void setAPCallback( std::function<void(WiFiManager*)>) { }
    const char* getConfigPortalSSID() { return "127.0.0.1"; }
    bool autoConnect(char const *apName, char const *apPassword = NULL) { return true; }
    void setDarkMode(bool) { }
    void setHostname(const char*) { }
    void process() { }
    void startWebPortal() { }
    bool addParameter(WiFiManagerParameter* p) { return true; }
    void setSaveParamsCallback(std::function<void()>) { }
    void setMenu(std::vector<const char*>& menu) { }
    void setTitle(String title) { }
    void setShowInfoErase(bool enabled) { }
    void setCustomMenuHTML(const char* html) { }
    
    std::unique_ptr<WebServer> server;
};
#endif

namespace mil {

enum class ErrorCondition { Note, Warning, Fatal };

enum ErrorType {
    AssertOutOfMem = 0x01,
    AssertPureVirtual = 0x02,
    AssertDeleteNotSupported = 0x03,
    AssertSingleApp = 0x04,
    AssertNoApp = 0x05,
    
    AssertSingleADC = 0x10,
    AssertSingleUSART0 = 0x11,
    AssertFixedPointBufferTooSmall = 0x12,
    
    AssertSingleTimer0 = 0x20,
    AssertSingleTimer1 = 0x21,
    AssertSingleTimer2 = 0x22,
    AssertSingleTimerEventMgr = 0x23,
    AssertNoTimerEventMgr = 0x24,
    AssertNoEventTimers = 0x25,
    
    AssertEthernetBadLength = 0x30,
    AssertEthernetNotInHandler = 0x31,
    AssertEthernetCannotSendData = 0x32,
    AssertEthernetNotWaitingToSendData = 0x33,
    AssertEthernetTransmitError = 0x34,
    AssertEthernetReceiveError = 0x35,
    
    AssertI2CReceiveNoBytes = 0x40,
    AssertI2CReceiveWrongSize = 0x41,
    AssertI2CSendBufferTooBig = 0x48,
    AssertI2CSendNoAckOnAddress = 0x49,
    AssertI2CSendNoAckOnData = 0x4A,
    AssertI2CSendError = 0x4B,
    
    AssertButtonTooMany = 0x50,
    AssertButtonOutOfRange = 0x51,
    AssertMenuHitEnd = 0x52,
    
    ErrorUser = 0x80,
};

static constexpr const char* endl = "\n";

void _showErrorCondition(char c, uint32_t code, enum ErrorCondition type);

#define ASSERT(expr, code) if (!(expr)) FATAL(code)
#define FATAL(code) _showErrorCondition(0, code, ErrorConditionFatal)
#define WARNING(code) _showErrorCondition(0, code, ErrorConditionWarning)

inline uint16_t makeuint16(uint8_t a, uint8_t b)
{
	return (static_cast<uint16_t>(a) << 8) | static_cast<uint16_t>(b);
}

inline void NOTE(uint16_t code) { _showErrorCondition(0, code, ErrorCondition::Note); }
inline void NOTE(uint8_t code1, uint8_t code2) { _showErrorCondition(0, makeuint16(code1, code2), ErrorCondition::Note); }
inline void CNOTE(char c, uint8_t code) { _showErrorCondition(c, code, ErrorCondition::Note); }

inline uint8_t toHexNibble(uint8_t n)
{
    uint8_t nibble = (n & 0x0f) + '0';
    return (nibble <= '9') ? nibble : (nibble - '0' - 10 + 'a');
}

// buf must have space for 2 characters. Result does not add a terminating '\0'
inline void toHex(uint8_t n, char* buf)
{
    buf[0] = toHexNibble(n >> 4);
    buf[1] = toHexNibble(n & 0xf);
}

}

#ifdef ARDUINO
class OutputStream
{
public:
	OutputStream& operator << (const char* s) { Serial.print(s); return *this; }
	OutputStream& operator << (const Printable& p) { p.printTo(Serial); return *this; }
	OutputStream& operator << (const __FlashStringHelper* s) { Serial.print(s); return *this; }
	OutputStream& operator << (const String& s) { Serial.print(s); return *this; }
	OutputStream& operator << (int64_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (uint64_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (int32_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (uint32_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (int16_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (uint16_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (int8_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (uint8_t v) { Serial.print(v); return *this; }
	OutputStream& operator << (char v) { Serial.print(v); return *this; }
	OutputStream& operator << (float v) { Serial.print(v); return *this; }
	OutputStream& operator << (double v) { Serial.print(v); return *this; }
	OutputStream& operator << (bool v) { Serial.print(v ? "true" : "false"); return *this; }
};

extern OutputStream cout;
#endif

