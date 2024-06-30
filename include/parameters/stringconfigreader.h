
#pragma once

#ifndef _LIB_PARAMS_STRINGCONFIGREADER_H_
#define _LIB_PARAMS_STRINGCONFIGREADER_H_

#include <parameters/configreader.h>
#include <parameters/CString.hpp>


namespace parameters {

	// --------------------------------------------------------------------------------------------------

	class StringConfigReader: public ConfigReader {
	public:
		StringConfigReader(const char *data, size_t size);
		StringConfigReader(const uint8_t *data, size_t size);
		StringConfigReader(const char *data_string);
		StringConfigReader(const std::string &data);
		virtual ~StringConfigReader();

		virtual bool ReadInfoLine(line &line) override;

	private:
		CString<> _buffer;
	};

}

#endif
