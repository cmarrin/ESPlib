/*-------------------------------------------------------------------------
This source file is a part of WiFiPortal

For the latest info, see http://www.marrin.org/

Copyright (c) 2025, Chris Marrin
All rights reserved.
-------------------------------------------------------------------------*/

// This code was adapted from:
//
//      https://github.com/FooBarWidget/multipart-parser
//
// It is heavily edited to bring it up to modern C++ standards and 
// to make it prettier :-)
//

#pragma once

#include <sys/types.h>
#include <string>
#include <array>
#include <functional>

class MultipartParser
{
  public:
    using Callback = std::function<void(const char* buffer, size_t start, size_t end, void* userData)>;

	MultipartParser() { reset(); }
	MultipartParser(const std::string &boundary) { setBoundary(boundary); }
	
	~MultipartParser() { delete[] _lookbehind; }
	
	void reset();
	
	void setBoundary(const std::string &boundary);
	
	size_t feed(const char *buffer, size_t len);
	
	bool succeeded() const { return _state == State::END; }
	
	bool hasError() const { return _state == State::ERROR; }
	
	bool stopped() const { return _state == State::ERROR || _state == State::END; }
	
	const char *getErrorMessage() const { return _errorReason; }
    
private:
	static const char CR     = 13;
	static const char LF     = 10;
	static const char SPACE  = 32;
	static const char HYPHEN = 45;
	static const char COLON  = 58;
	static const size_t UNMARKED = (size_t) -1;
	
	enum class State {
		ERROR,
		START,
		START_BOUNDARY,
		HEADER_FIELD_START,
		HEADER_FIELD,
		HEADER_VALUE_START,
		HEADER_VALUE,
		HEADER_VALUE_ALMOST_DONE,
		HEADERS_ALMOST_DONE,
		PART_DATA_START,
		PART_DATA,
		PART_END,
		END
	};
	
	enum Flags {
		PART_BOUNDARY = 1,
		LAST_BOUNDARY = 2
	};
	
	std::string _boundary;
	const char* _boundaryData = nullptr;
	size_t _boundarySize;
	std::array<bool, 256> _boundaryIndex;
	char* _lookbehind = nullptr;
	size_t _lookbehindSize;
	State _state;
	int _flags;
	size_t _index;
	size_t _headerFieldMark;
	size_t _headerValueMark;
	size_t _partDataMark;
	const char* _errorReason;

	Callback _onPartBegin = nullptr;
	Callback _onHeaderField = nullptr;
	Callback _onHeaderValue = nullptr;
	Callback _onHeaderEnd = nullptr;
	Callback _onHeadersEnd = nullptr;
	Callback _onPartData = nullptr;
	Callback _onPartEnd = nullptr;
	Callback _onEnd = nullptr;
	void* _userData = nullptr;
	
	void indexBoundary();
	
	void callback(Callback cb, const char *buffer = NULL, size_t start = UNMARKED,
		size_t end = UNMARKED, bool allowEmpty = false);
	
	void dataCallback(Callback cb, size_t &mark, const char *buffer, size_t i, size_t bufferLen,
		bool clear, bool allowEmpty = false);
        
	char lower(char c) const { return c | 0x20; }
	
	bool isBoundaryChar(char c) const { return _boundaryIndex[(unsigned char) c]; }
	
	bool isHeaderFieldCharacter(char c) const {
		return (c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| c == HYPHEN;
	}
	
	void setError(const char *message) {
		_state = State::ERROR;
		_errorReason = message;
	}
	
	void processPartData(size_t &prevIndex, size_t &index, const char *buffer,
		size_t len, size_t boundaryEnd, size_t &i, char c, State &state, int &flags);
};
