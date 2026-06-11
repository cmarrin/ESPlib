/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch and https://github.com/squix78/json-streaming-parser
*/

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
