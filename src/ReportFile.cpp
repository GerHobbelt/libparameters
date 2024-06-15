
#include <parameters/ReportFile.hpp>

#include "internal_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ReportFile
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// permanent lookup table:
	std::vector<std::string> ReportFile::_processed_file_paths;


	ReportFile::ReportFile(const char *path)
	{
		if (!path || !*path) {
			_f = nullptr;
			return;
		}

		_f = nullptr;

		if (strieq(path, "/dev/stdout") || strieq(path, "stdout") || strieq(path, "-") || strieq(path, "1"))
			_f = stdout;
		else if (strieq(path, "/dev/stderr") || strieq(path, "stderr") || strieq(path, "+") || strieq(path, "2"))
			_f = stderr;
		else {
			bool first = true;
			for (std::string &i : _processed_file_paths) {
				if (strieq(i.c_str(), path)) {
					first = false;
					break;
				}
			}
			_f = fopenUtf8(path, first ? "w" : "a");
			if (!_f) {
				tprintError("Cannot produce parameter usage report file: '{}'\n", path);
			} else if (first) {
				_processed_file_paths.push_back(path);
			}
		}
	}

	ReportFile::~ReportFile() {
		if (_f) {
			if (_f != stdout && _f != stderr) {
				fclose(_f);
			} else {
				fflush(_f);
			}
		}
	}

	FILE * ReportFile::operator()() const {
		return _f;
	}

}	// namespace
