//
// string class which offers a special feature vs. std::string:
// - the internal buffer can be read/WRITE accessed by external C/C++ code via the data() method,
//   allowing userland code to use arbitrary, fast, C- code to edit the string content,
//   including, f.e., injecting NUL sentinels a la strtok() et al.
// - built-in whitespace trimming methods.
// - supports segmented string, i.e. a string buffer where the string contains some internal NIL bytes.
//   These can be erased using the FlattenZeroContent() API while trailing NIL bytes can be quickly
//   eradicated using the ZeroTrimRight() API.
//

#pragma once

#ifndef _LIB_PARAMS_CSTRING_H_
#define _LIB_PARAMS_CSTRING_H_

#include <libassert/assert.hpp>
#include <cstdint>
#include <string>
#include <ctype.h>
#include <stdexcept>

namespace parameters {

	template <size_t SMALL_STRING_ALLOCSIZE = 16>
	class CString {
	public:
		CString() noexcept:
			_buffer(_small_buffer),
			_allocsize(SMALL_STRING_ALLOCSIZE),
			_str_start_offset(0),
			_contentsize(0)
		{
			_small_buffer[0] = 0;
		}
		CString(size_t allocate_size, size_t alloc_offset = 0):
			_buffer(nullptr),
			_allocsize(allocate_size),
			_str_start_offset(alloc_offset),
			_contentsize(alloc_offset)
		{
			if (allocate_size <= SMALL_STRING_ALLOCSIZE) {
				_buffer = _small_buffer;
				_allocsize = SMALL_STRING_ALLOCSIZE;
			} else {
				_buffer = (char *)malloc(allocate_size * sizeof(_buffer[0]));
				if (!_buffer) {
					throw std::bad_alloc();
				}
			}
			if (_str_start_offset >= _allocsize) {
				throw std::out_of_range("CString: initial alloc_offset is out of acceptable range");
			}
			memset(_buffer, 0, _str_start_offset + 1);
		}
		CString(const char *str, size_t alloc_offset = 0):
			_buffer(nullptr),
			_allocsize(SMALL_STRING_ALLOCSIZE),
			_str_start_offset(alloc_offset)
		{
			if (!str || !*str) {
				_contentsize = _str_start_offset;
				if (_str_start_offset >= _allocsize) {
					_allocsize = _str_start_offset + SMALL_STRING_ALLOCSIZE;
				}
				if (_allocsize <= SMALL_STRING_ALLOCSIZE) {
					_buffer = _small_buffer;
					_allocsize = SMALL_STRING_ALLOCSIZE;
				} else {
					_buffer = (char *)malloc(_allocsize * sizeof(_buffer[0]));
					if (!_buffer) {
						throw std::bad_alloc();
					}
				}
				memset(_buffer, 0, _str_start_offset + 1);
			} else {
				size_t slen = strlen(str);
				_contentsize = slen  + _str_start_offset;
				_allocsize = _contentsize + 1;
				if (_allocsize <= SMALL_STRING_ALLOCSIZE) {
					_buffer = _small_buffer;
					_allocsize = SMALL_STRING_ALLOCSIZE;
				} else {
					_buffer = (char *)malloc(_allocsize * sizeof(_buffer[0]));
					if (!_buffer) {
						throw std::bad_alloc();
					}
				}
				memset(_buffer, 0, _str_start_offset);
				strcpy(data(), str);
			}
		}
		CString(const std::string &str, size_t alloc_offset = 0):
			CString(str.c_str(), alloc_offset)
		{
		}

		~CString() {
			if (_buffer != _small_buffer) {
				free(_buffer);
			}
		}

		// trim leading and trailing whitespace.
		void Trim() {
			TrimLeft();
			TrimRight();
		}
		// trim leading whitespace.
		void TrimLeft() {
			char* p = data();
			while (std::isspace(*p))
				p++;
			_str_start_offset = p - _buffer;
		};

		// Return the number of newlines (LF chars) in the trimmed lead.
		unsigned int TrimLeftCountingNewlines() {
			const char *p = data();
			unsigned int lf_count = 0;
			while (*p && std::isspace(*p)) {
				if (*p == '\n')
					lf_count++;
				p++;
			}
			_str_start_offset = p - _buffer;
			return lf_count;
		}

		// trim trailing whitespace.
		void TrimRight() {
			char* p = data();
			char *e = p + length();
			e--;
			while (e >= p && (std::isspace(*e) || !*e))
				*e--;
			e++;
			*e = 0;
			// equivalent to `adjust_length(e - p)`:
			_contentsize = _str_start_offset + (e - p);
		}

		// trim trailing NUL bytes.
		void ZeroTrimRight() {
			char* p = data();
			char *e = p + length();
			e--;
			while (e >= p && !*e)
				*e--;
			e++;
			// equivalent to `adjust_length(e - p)`:
			_contentsize = _str_start_offset + (e - p);
		}

		// remove all NUL bytes within the active content block
		void FlattenZeroContent() {
			char *d = data();
			char *e = d + length();
			while (e > d && *d)
				d++;
			if (e > d) {
				// we've hit our first internal NIL: from now on source and target `d` will be offset...
				const char *t = d++;
				do {
					// jump over (first/next) NIL area; we're guaranteed this is at least size 1:
					while (e > d && !*d)
						d++;
					if (e > d) {
						// yes, those were internal NIL bytes, not just a bunch of sentinels!
						while (e > d && *d)
							*t++ = *d++;
					}
				} while (e > d);
				*t = 0;
				d = t;
			}
			// equivalent to `adjust_length(e - d)`:
			_contentsize = _str_start_offset + (e - d);
		}

		// return a pointer to the internal string buffer. Contrary to std::string::data(), this pointer points to an editable area of datasize() bytes.
		char *data() {
			return _buffer + _str_start_offset;
		}
		const char *data() const {
			return _buffer + _str_start_offset;
		}

		// return the number of bytes available in the internal string buffer. 
		size_t datasize() const {
			return _allocsize - _str_start_offset;
		}

		size_t allocsize() const {
			return _allocsize;
		}

		const char *c_str() const {
			return _buffer + _str_start_offset;
		}
		size_t length() const {
			return _contentsize - _str_start_offset; // strlen(data());
		}

		bool empty() const {
			return (length() == 0);
		}

		// force adjust the current data length; the data areawill be padded with NUL bytes to keep the string content intact.
		void adjust_length(size_t new_content_length) {
			size_t l = length();
			if (new_content_length < l) {
				// shrink!
				_contentsize = _str_start_offset + new_content_length;
				_buffer[_contentsize] = 0;
			} else if (new_content_length > l) {
				// grow! pad the content with NUL bytes!
				size_t new_contentsize = _str_start_offset + new_content_length;
				if (new_contentsize >= _allocsize) {
					resize(new_contentsize - _str_start_offset);
				}
				memset(_buffer + _contentsize, 0, new_contentsize + 1 - _contentsize);
				_contentsize = new_contentsize;
			}
		}

		// adjust the current data content length to match the string length pointed at by data() in the buffer.
		// The string length is bounded by datasize() as there can't be any more content in the buffer ATM;
		// this can help solve issues where userland code (some C and OS-specific APIs, for example) can produce
		// strings in buffers while not NUL-terminating them: we *always* have a NUL sentinel immediately after
		// the live content!
		void auto_adjust_length() {
			size_t l = strnlen(data(), datasize());
			adjust_length(l);
		}

		// Move the start index into the active part of the CString buffer forward (+) or backward (-) `count` characters.
		//
		// Scrolling the cursor like this is bounded by the buffered content: you cannot scroll past the end of the content
		// nor can you scroll beyond the start of the buffer (start_offset).
		void shift_start(int count) {
			if (count < 0) {
				size_t offset = -count;
				if (_str_start_offset >= offset) {
					_str_start_offset -= offset;
				} else {
					_str_start_offset = 0;
				}
			} else {
				size_t sl = length();
				size_t offset = count;
				if (offset >= sl) {
					_str_start_offset += sl;
					LIBASSERT_ASSERT(_buffer[_str_start_offset] == 0);
				} else {
					_str_start_offset += offset;
				}
			}
		}

		size_t get_current_shift() {
			return _str_start_offset;
		}

		// Move the start index into the active part of the CString buffer all the way back to the start of the buffer.
		// Equivalent to `shift_start(-get_current_shift())`.
		void reset_start() {
			_str_start_offset = 0;
		}

		void clear() {
			*_buffer = 0;
			_str_start_offset = 0;
			_contentsize = 0;
		}

		// move the active content to the start of the buffer, i.e. remove the current shoft.
		void unshift_content() {
			if (_str_start_offset == 0)
				return;
			if (empty()) {
				_str_start_offset = 0;
				_contentsize = 0;
				*_buffer = 0;
				return;
			}
			ZeroTrimRight();
			size_t sl = length();
			memmove(_buffer, data(), sl + 1);
			_contentsize = sl;
			_str_start_offset = 0;
		}

		bool startsWithAny(const char * prefix_set) {
			const char *s = data();
			return (nullptr != strchr(prefix_set, *s));
		}

		bool startsWith(const char *prefix) {
			const char *s = data();
			size_t pl = strlen(prefix);
			return (0 == strncmp(s, prefix, pl));
		}

		bool startsWith(char prefix) {
			const char *s = data();
			return (prefix == *s);
		}

		bool endsWith(const char *suffix) const {
			size_t sl = strlen(suffix);
			size_t bl = length();
			if (bl < sl)
				return false;
			return (0 == strcmp(c_str() + bl - sl, suffix));
		}

		bool endsWith(char suffix) const {
			size_t bl = length();
			if (bl < 1)
				return false;
			const char *e = c_str() + bl - 1;
			return (suffix == *e);
		}

		bool endsWithAny(const char *suffix_set) const {
			size_t bl = length();
			if (bl < 1)
				return false;
			const char *e = c_str() + bl - 1;
			return (nullptr != strchr(suffix_set, *e));
		}

		void resize(size_t alloc_size) {
			// do not clip content: adjust size if necessary:
			size_t slen = length() + _str_start_offset + 1;
			if (alloc_size < slen) {
				alloc_size = slen;
			}
			if (alloc_size <= SMALL_STRING_ALLOCSIZE) {
				if (_buffer != _small_buffer) {
					memcpy(_small_buffer, _buffer, slen);
					free(_buffer);
					_buffer = _small_buffer;
					_allocsize = SMALL_STRING_ALLOCSIZE;
				}
			} else {
				_allocsize = alloc_size;
				if (_buffer == _small_buffer) {
					_buffer = (char *)malloc(_allocsize * sizeof(_buffer[0]));
					if (!_buffer) {
						throw std::bad_alloc();
					}
					memcpy(_buffer, _small_buffer, slen);
				} else {
					_buffer = (char *)realloc(_buffer, _allocsize * sizeof(_buffer[0]));
					if (!_buffer) {
						throw std::bad_alloc();
					}
				}
			}
		}

		void append(const char *str) {
			if (!str || !*str)
				return;
			size_t dlen = length();
			size_t slen = strlen(str) + 1;
			if (datasize() - dlen < slen) {
				resize(_allocsize + slen);
			}
			strcpy(data() + dlen, str);
			_contentsize += slen - 1;
		}
		void append(const CString &str) {
			append(str.c_str());
		}
		void append(const std::string &str) {
			append(str.c_str());
		}

		void append(const char *data_ptr, size_t size) {
			if (!data_ptr || !size)
				return;
			size_t dlen = length();
			size_t slen = size + 1;
			if (datasize() - dlen < slen) {
				resize(_allocsize + slen);
			}
			char *d = data() + dlen;
			memcpy(d, data_ptr, size);
			d[size] = 0;
			_contentsize += size;
		}

		void append(const uint8_t *data_ptr, size_t size) {
			append((const char *)data_ptr, size);
		}

		// same as clear() + append(): rewinds the CString buffer to the start and appends the given data.
		void assign(const char *data_ptr, size_t size) {
			clear();
			append(data_ptr, size);
		}

		void assign(const uint8_t *data_ptr, size_t size) {
			clear();
			append(data_ptr, size);
		}

		void assign(const char *data_string) {
			clear();
			append(data_string);
		}

		void assign(const std::string &data) {
			clear();
			append(data.c_str());
		}

	protected:
		char _small_buffer[SMALL_STRING_ALLOCSIZE];
		size_t _allocsize;
		size_t _str_start_offset;
		size_t _contentsize;
		char *_buffer;
	};

}

#endif
