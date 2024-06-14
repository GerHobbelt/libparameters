//
// string class which offers a special feature vs. std::string:
// - the internal buffer can be read/WRITE accessed by external C/C++ code via the data() method,
//   allowing userland code to use arbitrary, fast, C- code to edit the string content,
//   including, f.e., injecting NUL sentinels a la strtok() et al.
// - built-in whitespace trimming methods.
//

#pragma once

#ifndef _LIB_PARAMS_CONFIGFILE_H_
#define _LIB_PARAMS_CONFIGFILE_H_

#include <cstdint>
#include <string>
#include <vector>

namespace parameters {

	// --------------------------------------------------------------------------------------------------

	// A simple FILE/stdio wrapper class which supports reading from stdin or regular file.
	class ConfigFile {
	public:
		// Parse '-', 'stdin' and '1' as STDIN, or open a regular text file in UTF8 read mode.
		//
		// An error line is printed via `tprintf()` when the given path turns out not to be valid.
		ConfigFile(const char *path);
		ConfigFile(const std::string &path)
			: ConfigFile(path.c_str()) {}
		~ConfigFile();

		FILE *operator()() const;

		operator bool() const {
			return _f != nullptr;
		};

	private:
		FILE *_f;
	};

}

#endif
