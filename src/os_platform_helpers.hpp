
#pragma once

#include <libassert/assert.hpp>		// we use libassert for our assertions

#include <fmt/base.h>
#include <fmt/format.h>

#include <spdlog/spdlog.h>

#include <ghc/fs_std.hpp>  // namespace fs = std::filesystem;   or   namespace fs = ghc::filesystem;

#include <climits> // for INT_MIN, INT_MAX
#include <cmath>   // for NAN, std::isnan
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale>  // for std::locale::classic
#include <sstream> // for std::stringstream
#include <functional>
#include <exception>
#include <cctype>  // for std::toupper
#include <type_traits>
#include <cerrno>
#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <memory>
#include <algorithm>


#ifdef _WIN32
#define NOMINMAX
#  include <windows.h>
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#else
#  include <strings.h>
#endif


#ifndef E_OK
#define E_OK 0
#endif



namespace parameters {

	static inline bool strieq(const char *s1, const char *s2) {
		return strcasecmp(s1, s2) == 0;
	}

	static inline bool SafeAtoi(const char* str, int* val) {
		char* endptr = nullptr;
		*val = strtol(str, &endptr, 10);
		return endptr != nullptr && *endptr == '\0';
	}

	static inline bool is_legal_fpval(double val) {
		return !std::isnan(val) && val != HUGE_VAL && val != -HUGE_VAL;
	}

	static inline bool is_single_word(const char* s) {
		if (!*s)
			return false;
		while (isalpha(*s))
			s++;
		while (isspace(*s))
			s++;
		return (!*s); // string must be at the end now...
	}

	static inline bool is_optional_whitespace(const char* s) {
		if (!*s)
			return false;
		while (isspace(*s))
			s++;
		return (!*s); // string must be at the end now...
	}

}   // namespace

