
#include <parameters/ConfigFile.hpp>

#include "internal_helpers.hpp"

#include <ghc/fs_std.hpp>  // namespace fs = std::filesystem;   or   namespace fs = ghc::filesystem;


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
			fs::path p = fs::weakly_canonical(path);
			std::u8string p8 = p.u8string();
			std::string ps = reinterpret_cast<const char *>(p8.c_str());
			_f = fopen(ps.c_str(), "r");
			if (!_f) {
				tprintError("Cannot open file for reading its content: {}\n", ps);
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
