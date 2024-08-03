
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {





	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamsReportWriter, et al
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ParamsReportDefaultWriter: public ParamsReportWriter {
	public:
		ParamsReportDefaultWriter(): ParamsReportWriter(nullptr) {}
		virtual ~ParamsReportDefaultWriter() = default;

		virtual void Write(const std::string &message) {
			PARAM_DEBUG("{}", message);
		}

	protected:
	};

	class ParamsReportFileDuoWriter: public ParamsReportWriter {
	public:
		ParamsReportFileDuoWriter(FILE *f): ParamsReportWriter(f) {
			is_separate_file_ = (f != nullptr && f != stderr && f != stdout);
		}
		virtual ~ParamsReportFileDuoWriter() = default;

		virtual void Write(const std::string &message) {
			// only write via tprintDebug() -- which usually logs to stderr -- when the `f` file destination is an actual file, rather than stderr or stdout.
			// This prevents these report lines showing up in duplicate on the console.
			if (is_separate_file_) {
				PARAM_DEBUG("{}", message);
			}
			size_t len = message.length();
			if (fwrite(message.c_str(), 1, len, file_) != len) {
				PARAM_ERROR("Failed to write params-report line to file. {}\n", strerror(errno));
			}
		}

	protected:
		bool is_separate_file_;
	};


	class ParamsReportStringWriter: public ParamsReportWriter {
	public:
		ParamsReportStringWriter(): ParamsReportWriter(nullptr) {}
		virtual ~ParamsReportStringWriter() = default;

		virtual void Write(const std::string message) {
			buffer += message;
		}

		std::string to_string() const {
			return buffer;
		}

	protected:
		std::string buffer;
	};












}	// namespace
