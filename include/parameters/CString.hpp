//
// string class which offers a special feature vs. std::string:
// - the internal buffer can be read/WRITE accessed by external C/C++ code via the data() method,
//   allowing userland code to use arbitrary, fast, C- code to edit the string content,
//   including, f.e., injecting NUL sentinels a la strtok() et al.
// - built-in whitespace trimming methods.
//

#pragma once

#ifndef _LIB_PARAMS_CSTRING_H_
#define _LIB_PARAMS_CSTRING_H_

#include <cstdint>
#include <string>
#include <ctype.h>

namespace parameters {

	template <size_t SMALL_STRING_ALLOCSIZE = 16>
	class CString {
	public:
		CString() noexcept :
			buffer(small_buffer),
			allocsize(sizeof(small_buffer)),
			str_start_offset(0)
		{
			small_buffer[0] = 0;
		}
		CString(size_t allocate_size) :
			buffer(nullptr),
			allocsize(allocate_size),
			str_start_offset(0)
		{
			if (allocate_size <= SMALL_STRING_ALLOCSIZE) {
				buffer = small_buffer;
			} else {
				buffer = new char[allocate_size];
			}
			buffer[0] = 0;
		}
		CString(const char *str) :
			buffer(nullptr),
			allocsize(sizeof(small_buffer)),
			str_start_offset(0)
		{
			if (!str || !*str) {
				buffer = small_buffer;
				buffer[0] = 0;
			} else {
				size_t slen = strlen(str);
				if (slen < SMALL_STRING_ALLOCSIZE) {
					buffer = small_buffer;
				} else {
					buffer = new char[slen + 1];
				}
				strcpy(buffer, str);
			}
		}
		CString(const std::string &str):
			buffer(nullptr),
			allocsize(sizeof(small_buffer)),
			str_start_offset(0)
		{
			if (str.empty()) {
				buffer = small_buffer;
				buffer[0] = 0;
			} else {
				size_t slen = str.size();
				if (slen < SMALL_STRING_ALLOCSIZE) {
					buffer = small_buffer;
				} else {
					buffer = new char[slen + 1];
				}
				strcpy(buffer, str.c_str());
			}
		}

		~CString() {
			if (buffer != small_buffer) {
				delete[] buffer;
			}
		}

		// trim leading and trailing whitespace.
		void Trim() {
			TrimLeft();
			TrimRight();
		}
		void TrimLeft() {
			char* p = data();
			while (isspace(*p))
				p++;
			str_start_offset = p - buffer;
		};
		void TrimRight() {
			char* p = data();
			char *e = p + length();
			e--;
			while (e >= p && isspace(*e))
				e--;
			e++;
			*e = 0;
		}

		char *data() {
			return buffer + str_start_offset;
		}
		size_t datasize() const {
			return allocsize - str_start_offset;
		}

		const char *c_str() const {
			return buffer + str_start_offset;
		}
		size_t length() const {
			return strlen(buffer + str_start_offset);
		}

	protected:
		char small_buffer[SMALL_STRING_ALLOCSIZE];
		size_t allocsize;
		size_t str_start_offset;
		char *buffer;
	};

}

#endif
