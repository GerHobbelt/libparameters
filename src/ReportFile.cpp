
#include <parameters/ReportFile.hpp>

#include "internal_helpers.hpp"

#include <ghc/fs_std.hpp>  // namespace fs = std::filesystem;   or   namespace fs = ghc::filesystem;


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
			const char *mode = (first ? "w" : "a");
			fs::path p = fs::weakly_canonical(path);
			std::u8string p8 = p.u8string();
			std::string ps = reinterpret_cast<const char *>(p8.c_str());
			_f = fopen(ps.c_str(), mode);
			if (!_f) {
				tprintError("Cannot produce report/output file: {}\n", ps);
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
