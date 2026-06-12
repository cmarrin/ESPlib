/*-------------------------------------------------------------------------
    This source file is a part of ESPlib
    For the latest info, see https://github.com/cmarrin/ESPlib
    Copyright (c) 2021-2025, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
    
    Original code is Copyright (c) 2015 by Daniel Eichhorn. See:

        http://blog.squix.ch
        https://github.com/squix78/json-streaming-parser

    The code has been significantly altered to follow the ESPlib
    style guide and to have more C++ style functionality. I've 
    also added the ability to parse from a file into a nested 
    std::map or std::vector, or a std::string if that's all there 
    is in the JSON. The parsed file is returned as a JSONParser::Value
    which is a tagged union of these 3 types
-------------------------------------------------------------------------*/

#pragma once

#include "mil.h"

namespace mil {

class JSONParser {
  public:
    JSONParser();
    bool parse(char c);
    void reset();
    std::string errorString()
    {
        return _errorString + " at line " + std::to_string(_currentLine) + ", char " + std::to_string(_currentChar);
    }

  protected:
    virtual void handleWhitespace(char c) { }
    virtual void handleStartDocument() { }
    virtual void handleKey(const std::string& key) { }
    virtual void handleValue(const std::string& value) { }
    virtual void handleEndArray() { }
    virtual void handleEndObject() { }
    virtual void handleEndDocument() { }
    virtual void handleStartArray() { }
    virtual void handleStartObject() { }

  private:
    static constexpr int BufferSize = 512;
    
    enum class State {
        StartDocument,
        Done,
        InArray,
        InObject,
        EndKey,
        AfterKey,
        InString,
        StartEscape,
        Unicode,
        InNumber,
        InTrue,
        InFalse,
        InNull,
        AfterValue,
        UnicodeSurrogate,
    };
    
    enum class Stack { Object, Array, Key, String };

    void increaseBufferPointer();
    bool endString();
    bool endArray();
    bool startValue(char c);
    void startKey();
    bool processEscapeCharacters(char c);
    bool isDigit(char c);
    bool isHexCharacter(char c);
    char convertCodepointToCharacter(int num);
    void endUnicodeCharacter(int codepoint);
    void startNumber(char c);
    void startString();
    void startObject();
    void startArray();
    bool endNull();
    bool endFalse();
    bool endTrue();
    void endDocument();
    int convertDecimalBufferToInt(char myArray[], int length);
    void endNumber();
    bool endUnicodeSurrogateInterstitial();
    bool doesCharArrayContain(char myArray[], int length, char c);
    int getHexArrayAsDecimal(char hexArray[], int length);
    bool processUnicodeCharacter(char c);
    bool endObject();
    
    State _state;
    std::vector<Stack> _stack;

    bool _doEmitWhitespace = false;
    
    char _buffer[BufferSize];
    int _bufferPos = 0;

    char _unicodeEscapeBuffer[10];
    int _unicodeEscapeBufferPos = 0;

    char _unicodeBuffer[10];
    int _unicodeBufferPos = 0;
    int _unicodeHighSurrogate = 0;
    
    int _currentLine = 1;
    int _currentChar = 1;
    std::string _errorString;
};

}
