
#include <parameters/parameters.h>

#include "internal_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamHash
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Note about Param names, i.e. Variable Names:
	// 
	// - accept both `-` and `_` in key names, e.g. user-specified 'debug-all' would match 'debug_all'
	//   in the database.
	// - names are matched case-*in*sensitive and must be ASCII. Unicode characters in Variable Names 
	//   are not supported.

	// calculate hash:
	std::size_t ParamHash::operator()(const Param& s) const noexcept {
		const char * str = s.name_str();
		return ParamHash()(str);
	}
	// calculate hash:
	std::size_t ParamHash::operator()(const char * s) const noexcept {
		DEBUG_ASSERT(s != nullptr);
		uint32_t h = 1;
		for (const char *p = s; *p; p++) {
			uint32_t c = std::toupper(static_cast<unsigned char>(*p));
			if (c == '-')
				c = '_';
			h *= 31397;
			h += c;
		}
		return h;
	}

	// equal_to:
	bool ParamHash::operator()(const Param& lhs, const Param& rhs) const noexcept {
		DEBUG_ASSERT(lhs.name_str() != nullptr);
		DEBUG_ASSERT(rhs.name_str() != nullptr);
		return ParamHash()(lhs.name_str(), rhs.name_str());
	}
	// equal_to:
	bool ParamHash::operator()(const char * lhs, const char * rhs) const noexcept {
		DEBUG_ASSERT(lhs != nullptr);
		DEBUG_ASSERT(rhs != nullptr);
		for (; *lhs; lhs++, rhs++) {
			uint32_t c = std::toupper(static_cast<unsigned char>(*lhs));
			if (c == '-')
				c = '_';
			uint32_t d = std::toupper(static_cast<unsigned char>(*rhs));
			if (d == '-')
				d = '_';
			if (c != d)
				return false;
		}
		DEBUG_ASSERT(*lhs == 0);
		return *rhs == 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamComparer
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// compare as a-less-b? for purposes of std::sort et al:
	bool ParamComparer::operator()(const Param& lhs, const Param& rhs) const {
		return ParamComparer()(lhs.name_str(), rhs.name_str());
	}
	// compare as a-less-b? for purposes of std::sort et al:
	bool ParamComparer::operator()(const char * lhs, const char * rhs) const {
		DEBUG_ASSERT(lhs != nullptr);
		DEBUG_ASSERT(rhs != nullptr);
		for (; *lhs; lhs++, rhs++) {
			uint32_t c = std::toupper(static_cast<unsigned char>(*lhs));
			if (c == '-')
				c = '_';
			uint32_t d = std::toupper(static_cast<unsigned char>(*rhs));
			if (d == '-')
				d = '_';
			// long names come before short names; otherwise sort A->Z
			if (c != d)
				return d == 0 ? true : (c < d);
		}
		DEBUG_ASSERT(*lhs == 0);
		return *rhs == 0;
	}

} // namespace tesseract
