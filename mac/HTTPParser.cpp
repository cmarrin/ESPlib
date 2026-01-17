/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

#ifndef ARDUINO

#include "HTTPParser.h"

void
HTTPParser::indexBoundary()
{
    const char *current;
    const char *end = _boundaryData + _boundarySize;
    
    std::fill_n(_boundaryIndex.begin(), _boundaryIndex.size(), false);
    
    for (current = _boundaryData; current < end; current++) {
        _boundaryIndex[(unsigned char) *current] = true;
    }
}

void
HTTPParser::callback(CBType type, const char *buffer, size_t start, size_t end, bool allowEmpty)
{
    if (start != UNMARKED && start == end && !allowEmpty) {
        return;
    }
    if (_callback) {
        _callback(type, buffer, start, end);
    }
}

void
HTTPParser::dataCallback(CBType type, size_t &mark, const char *buffer, size_t i, size_t bufferLen, bool clear, bool allowEmpty)
{
    if (mark == UNMARKED) {
        return;
    }
    
    if (!clear) {
        callback(type, buffer, mark, bufferLen, allowEmpty);
        mark = 0;
    } else {
        callback(type, buffer, mark, i, allowEmpty);
        mark = UNMARKED;
    }
}

void
HTTPParser::processPartData(size_t &prevIndex, size_t &index, const char *buffer,
    size_t len, size_t boundaryEnd, size_t &i, char c, State &state, int &flags)
{
    prevIndex = index;
    
    if (index == 0) {
        // boyer-moore derived algorithm to safely skip non-boundary data
        while (i + _boundarySize <= len) {
            if (isBoundaryChar(buffer[i + boundaryEnd])) {
                break;
            }
            
            i += _boundarySize;
        }
        if (i == len) {
            return;
        }
        c = buffer[i];
    }
    
    if (index < _boundarySize) {
        if (_boundary[index] == c) {
            if (index == 0) {
                dataCallback(CBType::PartData, _partDataMark, buffer, i, len, true);
            }
            index++;
        } else {
            index = 0;
        }
    } else if (index == _boundarySize) {
        index++;
        if (c == CR) {
            // CR = part boundary
            flags |= PART_BOUNDARY;
        } else if (c == HYPHEN) {
            // HYPHEN = end boundary
            flags |= LAST_BOUNDARY;
        } else {
            index = 0;
        }
    } else if (index - 1 == _boundarySize) {
        if (flags & PART_BOUNDARY) {
            index = 0;
            if (c == LF) {
                // unset the PART_BOUNDARY flag
                flags &= ~PART_BOUNDARY;
                callback(CBType::PartEnd);
                callback(CBType::PartBegin);
                state = State::HEADER_FIELD_START;
                return;
            }
        } else if (flags & LAST_BOUNDARY) {
            if (c == HYPHEN) {
                callback(CBType::PartEnd);
                callback(CBType::End);
                state = State::END;
            } else {
                index = 0;
            }
        } else {
            index = 0;
        }
    } else if (index - 2 == _boundarySize) {
        if (c == CR) {
            index++;
        } else {
            index = 0;
        }
    } else if (index - _boundarySize == 3) {
        index = 0;
        if (c == LF) {
            callback(CBType::PartEnd);
            callback(CBType::End);
            state = State::END;
            return;
        }
    }
    
    if (index > 0) {
        // when matching a possible boundary, keep a lookbehind reference
        // in case it turns out to be a false lead
        if (index - 1 >= _lookbehindSize) {
            setError("Parser bug: index overflows lookbehind buffer. "
                "Please send bug report with input file attached.");
            throw std::out_of_range("index overflows lookbehind buffer");
        } else if (index - 1 < 0) {
            setError("Parser bug: index underflows lookbehind buffer. "
                "Please send bug report with input file attached.");
            throw std::out_of_range("index underflows lookbehind buffer");
        }
        _lookbehind[index - 1] = c;
    } else if (prevIndex > 0) {
        // if our boundary turned out to be rubbish, the captured lookbehind
        // belongs to partData
        callback(CBType::PartData, _lookbehind, 0, prevIndex);
        prevIndex = 0;
        _partDataMark = i;
        
        // reconsider the current character even so it interrupted the sequence
        // it could be the beginning of a new sequence
        i--;
    }
}

void
HTTPParser::reset()
{
    delete[] _lookbehind;
    _state = State::ERROR;
    _boundary.clear();
    _boundaryData = _boundary.c_str();
    _boundarySize = 0;
    _lookbehind = nullptr;
    _lookbehindSize = 0;
    _flags = 0;
    _index = 0;
    _headerFieldMark = UNMARKED;
    _headerValueMark = UNMARKED;
    _partDataMark    = UNMARKED;
    _errorReason     = "Parser uninitialized.";
}

void
HTTPParser::setBoundary(const std::string &boundary)
{
    reset();
    _state = State::START;
    
    if (!boundary.empty()) {
        _boundary = "\r\n--" + boundary;
        _boundaryData = _boundary.c_str();
        _boundarySize = _boundary.size();
        indexBoundary();
        _lookbehind = new char[_boundarySize + 8];
        _lookbehindSize = _boundarySize + 8;
        _errorReason = "No error.";
    }
}

size_t
HTTPParser::feed(const char *buffer, size_t len) 
{
    if (_state == State::ERROR || len == 0) {
        return 0;
    }
    
    size_t i;
    char c, cl;
    size_t prevIndex = _index;
    size_t boundaryEnd = _boundarySize - 1;
    
    for (i = 0; i < len; i++) {
        c = buffer[i];
        
        switch (_state) {
        case State::ERROR:
            return i;
        case State::START:
            _index = 0;
            _state = State::START_BOUNDARY;
        case State::START_BOUNDARY:
            if (_index == _boundarySize - 2) {
                if (c != CR) {
                    setError("Malformed. Expected CR after boundary.");
                    return i;
                }
                _index++;
                break;
            } else if (_index - 1 == _boundarySize - 2) {
                if (c != LF) {
                    setError("Malformed. Expected LF after boundary CR.");
                    return i;
                }
                _index = 0;
                callback(CBType::PartBegin);
                _state = State::HEADER_FIELD_START;
                break;
            }
            if (c != _boundary[_index + 2]) {
                setError("Malformed. Found different boundary data than the given one.");
                return i;
            }
            _index++;
            break;
        case State::HEADER_FIELD_START:
            _state = State::HEADER_FIELD;
            _headerFieldMark = i;
            _index = 0;
        case State::HEADER_FIELD:
            if (c == CR) {
                _headerFieldMark = UNMARKED;
                _state = State::HEADERS_ALMOST_DONE;
                break;
            }

            _index++;
            if (c == HYPHEN) {
                break;
            }

            if (c == COLON) {
                if (_index == 1) {
                    // empty header field
                    setError("Malformed first header name character.");
                    return i;
                }
                dataCallback(CBType::HeaderField, _headerFieldMark, buffer, i, len, true);
                _state = State::HEADER_VALUE_START;
                break;
            }

            cl = lower(c);
            if (cl < 'a' || cl > 'z') {
                setError("Malformed header name.");
                return i;
            }
            break;
        case State::HEADER_VALUE_START:
            if (c == SPACE) {
                break;
            }
            
            _headerValueMark = i;
            _state = State::HEADER_VALUE;
        case State::HEADER_VALUE:
            if (c == CR) {
                dataCallback(CBType::HeaderValue, _headerValueMark, buffer, i, len, true, true);
                callback(CBType::HeaderEnd);
                _state = State::HEADER_VALUE_ALMOST_DONE;
            }
            break;
        case State::HEADER_VALUE_ALMOST_DONE:
            if (c != LF) {
                setError("Malformed header value: LF expected after CR");
                return i;
            }
            
            _state = State::HEADER_FIELD_START;
            break;
        case State::HEADERS_ALMOST_DONE:
            if (c != LF) {
                setError("Malformed header ending: LF expected after CR");
                return i;
            }
            
            callback(CBType::HeadersEnd);
            _state = State::PART_DATA_START;
            break;
        case State::PART_DATA_START:
            _state = State::PART_DATA;
            _partDataMark = i;
        case State::PART_DATA:
            processPartData(prevIndex, _index, buffer, len, boundaryEnd, i, c, _state, _flags);
            break;
        default:
            return i;
        }
    }
    
    dataCallback(CBType::HeaderField, _headerFieldMark, buffer, i, len, false);
    dataCallback(CBType::HeaderValue, _headerValueMark, buffer, i, len, false);
    dataCallback(CBType::PartData, _partDataMark, buffer, i, len, false);
    
    return len;
}
	
#endif
