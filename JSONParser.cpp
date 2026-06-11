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

#include "JSONParser.h"

#include <cstring>

using namespace mil;

JSONParser::JSONParser() {
    reset();
}

void
JSONParser::reset()
{
    _state = State::StartDocument;
    _bufferPos = 0;
    _unicodeEscapeBufferPos = 0;
    _unicodeBufferPos = 0;
    _currentLine = 1;
    _currentChar = 1;
    _errorString.clear();
}

bool
JSONParser::parse(char c)
{
    // Handle current line and char
    if (c == '\n') {
        _currentLine++;
        _currentChar = 1;
    } else {
        _currentChar++;
    }
    
    // valid whitespace characters in JSON (from RFC4627 for JSON) include:
    // space, horizontal tab, line feed or new line, and carriage return.
    // thanks:
    // http://stackoverflow.com/questions/16042274/definition-of-whitespace-in-json
    if ((c == ' ' || c == '\t' || c == '\n' || c == '\r')
        && !(_state == State::InString || _state == State::Unicode || _state == State::StartEscape
            || _state == State::InNumber || _state == State::StartDocument)) {
        return true;
    }
    
    switch (_state) {
        case State::InString:
            if (c == '"') {
                return endString();
            }
            if (c == '\\') {
                _state = State::StartEscape;
            } else if ((c < 0x1f) || (c == 0x7f)) {
                _errorString = "Unescaped control character encountered";
                return false;
            }
            _buffer[_bufferPos] = c;
            increaseBufferPointer();
            break;
        case State::InArray:
            if (c == ']') {
                return endArray();
            }
            return startValue(c);
        case State::InObject:
            if (c == '}') {
                if (!endObject()) {
                    return false;
                }
            } else if (c == '"') {
                startKey();
            } else {
                _errorString = "Start of string expected for object key";
                return false;
            }
            break;
        case State::EndKey:
            if (c != ':') {
                _errorString = "Expected ':' after key";
                return false;
            }
            _state = State::AfterKey;
            break;
        case State::AfterKey:
            return startValue(c);
        case State::StartEscape:
            return processEscapeCharacters(c);
        case State::Unicode:
            return processUnicodeCharacter(c);
        case State::UnicodeSurrogate:
            _unicodeEscapeBuffer[_unicodeEscapeBufferPos] = c;
            _unicodeEscapeBufferPos++;
            if (_unicodeEscapeBufferPos == 2) {
                return endUnicodeSurrogateInterstitial();
            }
            break;
        case State::AfterValue: {
            // not safe for size == 0!!!
            Stack within = _stack.back();
            if (within == Stack::Object) {
                if (c == '}') {
                    if (!endObject()) {
                        return false;
                    }
                } else if (c == ',') {
                    _state = State::InObject;
                } else {
                    _errorString = "Expected ',' or '}' while parsing object";
                    return false;
                }
            } else if (within == Stack::Array) {
                if (c == ']') {
                    if (!endArray()) {
                        return false;
                    }
                } else if (c == ',') {
                    _state = State::InArray;
                } else {
                    _errorString = "Expected ',' or ']' while parsing array";
                    return false;
                }
            } else {
                _errorString = "Finished a literal, but unclear what state to move to";
                return false;
            }
            break;
        }
        case State::InNumber:
            if (c >= '0' && c <= '9') {
                _buffer[_bufferPos] = c;
                increaseBufferPointer();
            } else if (c == '.') {
                if (doesCharArrayContain(_buffer, _bufferPos, '.')) {
                    _errorString = "Cannot have multiple decimal points in a number";
                    return false;
                } else if (doesCharArrayContain(_buffer, _bufferPos, 'e')) {
                    _errorString = "Cannot have a decimal point in an exponent";
                    return false;
                }
                _buffer[_bufferPos] = c;
                increaseBufferPointer();
            } else if (c == 'e' || c == 'E') {
                if (doesCharArrayContain(_buffer, _bufferPos, 'e')) {
                    _errorString = "Cannot have multiple exponents in a number";
                    return false;
                }
                _buffer[_bufferPos] = c;
                increaseBufferPointer();
            } else if (c == '+' || c == '-') {
                char last = _buffer[_bufferPos - 1];
                if (!(last == 'e' || last == 'E')) {
                    _errorString = "Can only have '+' or '-' after the 'e' or 'E' in a number";
                    return false;
                }
                _buffer[_bufferPos] = c;
                increaseBufferPointer();
            } else {
                endNumber();
                // we have consumed one beyond the end of the number
                parse(c);
            }
            break;
        case State::InTrue:
            _buffer[_bufferPos] = c;
            increaseBufferPointer();
            if (_bufferPos == 4) {
                return endTrue();
            }
            break;
        case State::InFalse:
            _buffer[_bufferPos] = c;
            increaseBufferPointer();
            if (_bufferPos == 5) {
                return endFalse();
            }
            break;
        case State::InNull:
            _buffer[_bufferPos] = c;
            increaseBufferPointer();
            if (_bufferPos == 4) {
                return endNull();
            }
            break;
        case State::StartDocument:
            handleStartDocument();
            if (c == '[') {
                startArray();
            } else if (c == '{') {
                startObject();
            } else {
                _errorString = "Document must start with object or array.";
                return false;
            }
            break;
        case State::Done:
            _errorString = "Expected end of document.";
            return false;
        default:
            _errorString = "Internal error. Reached an unknown state";
            return false;
    }

    return true;
}

void
JSONParser::increaseBufferPointer()
{
    if (_bufferPos + 1 < BufferSize - 1) {
        _bufferPos = _bufferPos + 1;
    } else {
        _bufferPos = BufferSize - 1;
    }
}

bool
JSONParser::endString()
{
    Stack popped = _stack.back();
    _stack.pop_back();
    if (popped == Stack::Key) {
        _buffer[_bufferPos] = '\0';
        handleKey(_buffer);
        _state = State::EndKey;
    } else if (popped == Stack::String) {
        _buffer[_bufferPos] = '\0';
        handleValue(_buffer);
        _state = State::AfterValue;
    } else {
        _errorString = "Unexpected end of string";
        return false;
    }
    _bufferPos = 0;
    return true;
}
  
bool
JSONParser::startValue(char c)
{
    if (c == '[') {
        startArray();
    } else if (c == '{') {
        startObject();
    } else if (c == '"') {
        startString();
    } else if (isDigit(c)) {
        startNumber(c);
    } else if (c == 't') {
        _state = State::InTrue;
        _buffer[_bufferPos] = c;
        increaseBufferPointer();
    } else if (c == 'f') {
        _state = State::InFalse;
        _buffer[_bufferPos] = c;
        increaseBufferPointer();
    } else if (c == 'n') {
        _state = State::InNull;
        _buffer[_bufferPos] = c;
        increaseBufferPointer();
    } else {
        _errorString = "Unexpected character for value";
        return false;
    }
    return true;
}

bool
JSONParser::isDigit(char c)
{
    // Only concerned with the first character in a number.
    return (c >= '0' && c <= '9') || c == '-';
}

bool
JSONParser::endArray()
{
    Stack popped = _stack.back();
    _stack.pop_back();
    if (popped != Stack::Array) {
        _errorString = "Unexpected end of array encountered";
        return false;
    }
    handleEndArray();
    _state = State::AfterValue;
    if (_stack.empty()) {
        endDocument();
    }
    return true;
}

void
JSONParser::startKey()
{
    _stack.push_back(Stack::Key);
    _state = State::InString;
}

bool
JSONParser::endObject()
{
    Stack popped = _stack.back();
    _stack.pop_back();
    if (popped != Stack::Object) {
        _errorString = "Unexpected end of object encountered";
        return false;
    }
    handleEndObject();
    _state = State::AfterValue;
    if (_stack.empty()) {
        endDocument();
    }
    return true;
}

bool
JSONParser::processEscapeCharacters(char c)
{
    if (c == '"') {
        _buffer[_bufferPos] = '"';
        increaseBufferPointer();
    } else if (c == '\\') {
        _buffer[_bufferPos] = '\\';
        increaseBufferPointer();
    } else if (c == '/') {
        _buffer[_bufferPos] = '/';
        increaseBufferPointer();
    } else if (c == 'b') {
        _buffer[_bufferPos] = 0x08;
        increaseBufferPointer();
    } else if (c == 'f') {
        _buffer[_bufferPos] = '\f';
        increaseBufferPointer();
    } else if (c == 'n') {
        _buffer[_bufferPos] = '\n';
        increaseBufferPointer();
    } else if (c == 'r') {
        _buffer[_bufferPos] = '\r';
        increaseBufferPointer();
    } else if (c == 't') {
        _buffer[_bufferPos] = '\t';
        increaseBufferPointer();
    } else if (c == 'u') {
        _state = State::Unicode;
    } else {
        _errorString = "Expected escaped character after backslash";
        return false;
    }
    if (_state != State::Unicode) {
        _state = State::InString;
    }
    return true;
}

bool
JSONParser::processUnicodeCharacter(char c)
{
    if (!isHexCharacter(c)) {
        _errorString = "Expected hex character for escaped Unicode character";
        return false;
    }

    _unicodeBuffer[_unicodeBufferPos] = c;
    _unicodeBufferPos++;

    if (_unicodeBufferPos == 4) {
        int codepoint = getHexArrayAsDecimal(_unicodeBuffer, _unicodeBufferPos);
        endUnicodeCharacter(codepoint);
    }
    return true;
}

bool
JSONParser::isHexCharacter(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int
JSONParser::getHexArrayAsDecimal(char hexArray[], int length)
{
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = hexArray[length - i - 1];
      int value = 0;
      if (current >= 'a' && current <= 'f') {
        value = current - 'a' + 10;
      } else if (current >= 'A' && current <= 'F') {
        value = current - 'A' + 10;
      } else if (current >= '0' && current <= '9') {
        value = current - '0';
      }
      result += value * 16^i;
    }
    return result;
}

bool
JSONParser::doesCharArrayContain(char myArray[], int length, char c)
{
    for (int i = 0; i < length; i++) {
      if (myArray[i] == c) {
        return true;
      }
    }
    return false;
}

bool
JSONParser::endUnicodeSurrogateInterstitial()
{
    char unicodeEscape = _unicodeEscapeBuffer[_unicodeBufferPos - 1];
    if (unicodeEscape != 'u') {
        _errorString = "Expected '\\u' following a Unicode high surrogate";
        return false;
    }
    _unicodeBufferPos = 0;
    _unicodeBufferPos = 0;
    _state = State::Unicode;
    return true;
}

void
JSONParser::endNumber()
{
    _buffer[_bufferPos] = '\0';
    handleValue(_buffer);
    _bufferPos = 0;
    _state = State::AfterValue;
}

int
JSONParser::convertDecimalBufferToInt(char myArray[], int length)
{
    int result = 0;
    for (int i = 0; i < length; i++) {
      char current = myArray[length - i - 1];
      result += (current - '0') * 10;
    }
    return result;
}

void
JSONParser::endDocument()
{
    handleEndDocument();
    _state = State::Done;
}

bool
JSONParser::endTrue()
{
    _buffer[_bufferPos] = '\0';
    if (strcmp(_buffer, "true") == 0) {
        handleValue("true");
    } else {
        _errorString = "Expected 'true'";
        return false;
    }
    _bufferPos = 0;
    _state = State::AfterValue;
    return true;
}

bool
JSONParser::endFalse()
{
    _buffer[_bufferPos] = '\0';
    if (strcmp(_buffer, "false") == 0) {
        handleValue("false");
    } else {
        _errorString = "Expected 'false'";
        return false;
    }
    _bufferPos = 0;
    _state = State::AfterValue;
    return true;
}

bool
JSONParser::endNull()
{
    _buffer[_bufferPos] = '\0';
    if (strcmp(_buffer, "null") == 0) {
        handleValue("null");
    } else {
        _errorString = "Expected 'null'";
        return false;
    }
    _bufferPos = 0;
    _state = State::AfterValue;
    return true;
}

void
JSONParser::startArray()
{
    handleStartArray();
    _state = State::InArray;
    _stack.push_back(Stack::Array);
}

void
JSONParser::startObject()
{
    handleStartObject();
    _state = State::InObject;
    _stack.push_back(Stack::Object);
}

void
JSONParser::startString()
{
    _stack.push_back(Stack::String);
    _state = State::InString;
}

void
JSONParser::startNumber(char c)
{
    _state = State::InNumber;
    _buffer[_bufferPos] = c;
    increaseBufferPointer();
}

void
JSONParser::endUnicodeCharacter(int codepoint)
{
    _buffer[_bufferPos] = convertCodepointToCharacter(codepoint);
    increaseBufferPointer();
    _unicodeBufferPos = 0;
    _unicodeHighSurrogate = -1;
    _state = State::InString;
}

char
JSONParser::convertCodepointToCharacter(int num)
{
    if (num <= 0x7F) {
        return (char) (num);
    }
    
    // if(num<=0x7FF) return (char)((num>>6)+192) + (char)((num&63)+128);
    // if(num<=0xFFFF) return
    // chr((num>>12)+224).chr(((num>>6)&63)+128).chr((num&63)+128);
    // if(num<=0x1FFFFF) return
    // chr((num>>18)+240).chr(((num>>12)&63)+128).chr(((num>>6)&63)+128).chr((num&63)+128);

    return ' ';
}
