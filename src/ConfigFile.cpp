
#include <parameters/ConfigFile.hpp>

#include "internal_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ConfigFile
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	ConfigFile::ConfigFile(const char *path)
	{
		if (!path || !*path) {
			_f = nullptr;
			return;
		}

		_f = nullptr;

		if (strieq(path, "/dev/stdin") || strieq(path, "stdin") || strieq(path, "-") || strieq(path, "1"))
			_f = stdin;
		else {
			_f = fopenUtf8(path, "r");
			if (!_f) {
				tprintError("Cannot open file: '{}'\n", path);
			}
		}
	}

	ConfigFile::~ConfigFile() {
		if (_f) {
			if (_f != stdin) {
				fclose(_f);
			} else {
				fflush(_f);
			}
		}
	}

	FILE *ConfigFile::operator()() const {
		return _f;
	}

}  // namespace
