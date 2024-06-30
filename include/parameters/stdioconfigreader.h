
#pragma once

#ifndef _LIB_PARAMS_STDIOCONFIGFILE_H_
#define _LIB_PARAMS_STDIOCONFIGFILE_H_

#include <parameters/configreader.h>

#include <cstdint>
#include <cstdio>
#include <string>

namespace parameters {

	// --------------------------------------------------------------------------------------------------

	// A simple FILE/stdio wrapper class which supports reading from stdin or regular file.
	class StdioConfigReader: public ConfigReader {
	public:
		// Parse '-', 'stdin' and '1' as STDIN, or open a regular text file in UTF8 read mode.
		//
		// An error line is printed via `tprintf()` when the given path turns out not to be valid.
		StdioConfigReader(const char *path);
		StdioConfigReader(const std::string &path);
		virtual ~StdioConfigReader();

		FILE *operator()() const;

		operator bool() const {
			return _f != nullptr;
		};

		virtual bool ReadInfoLine(line &line) override;

	private:
		FILE *_f;
	};

}

#endif
