
#include <parameters/parameters.h>

#include "internal_helpers.hpp"

#include <algorithm>


namespace parameters {

	template<typename T>
	static inline int sign_of_diff(T a, T b) {
		if (a == b)
			return 0;
		if (a < b)
			return -1;
		return 1;
	}

	static inline const char *type_as_str(ParamType type) {
		switch (type) {
		case INT_PARAM:
			return "[Integer]";
		case BOOL_PARAM:
			return "[Boolean]";
		case DOUBLE_PARAM:
			return "[Float]";
		case STRING_PARAM:
			return "[String]";

		case INT_SET_PARAM:
			return "[Arr:Int]";
		case BOOL_SET_PARAM:
			return "[Arr:Bool]";
		case DOUBLE_SET_PARAM:
			return "[Arr:Flt]";
		case STRING_SET_PARAM:
			return "[Arr:Str]";

		case CUSTOM_PARAM:
			return "[Custom]";
		case CUSTOM_SET_PARAM:
			return "[Arr:Cust]";

		case ANY_TYPE_PARAM:
			return "[ANY]";

		default:
			return "[???]";
		}
	}

	static inline int acc(int access) {
		if (access > 2)
			access = 2;
		return access;
		}

	static inline int clip(int access) {
		if (access > 999)
			access = 999;
		return access;
	}


	// Print all parameters in the given set(s) to the given output.
	void ParamUtils::PrintParams(ParamsReportWriter &dst, const ParamsVectorSet &set, const char *section_title) {
#if 0
		std::ostringstream stream;
		stream.imbue(std::locale::classic());
#endif
		if (!section_title || !*section_title)
			section_title = ParamUtils::GetApplicationName().c_str();

		dst.WriteReportHeaderLine(fmt::format("# {} parameters overview", section_title));

		for (ParamsVector *vec : set.get()) {
			LIBASSERT_DEBUG_ASSERT(vec != nullptr);

			dst.WriteReportHeaderLine(fmt::format("## {}", vec->title()));

			// sort the parameters by name, per vectorset / section:
			std::vector<ParamPtr> params = vec->as_list();
			std::sort(params.begin(), params.end(), [](const ParamPtr& a, const ParamPtr& b)
			{
				int rv = sign_of_diff(a->is_init(), b->is_init());
				if (rv == 0)
				{
					rv = sign_of_diff(b->is_debug(), a->is_debug());
					if (rv == 0)
					{
						rv = strcmp(b->name_str(), a->name_str());
#if !defined(NDEBUG)
						if (rv == 0)
						{
							LIBASSERT_PANIC(fmt::format("Apparently you have double-defined a {} Variable: '{}'! Fix that in the source code!\n", ParamUtils::GetApplicationName(), a->name_str()).c_str());
						}
#endif
					}
				}
				return (rv >= 0);
			});

			for (ParamPtr param : params) {
				dst.WriteParamInfo(*param);
				dst.WriteParamValue(*param);
			}
		}
	}

	// Report parameters' usage statistics, i.e. report which params have been
	// set, modified and read/checked until now during this run-time's lifetime.
	//
	// Use this method for run-time 'discovery' about which tesseract parameters
	// are actually *used* during your particular usage of the library, ergo
	// answering the question:
	// "Which of all those parameters are actually *relevant* to my use case today?"
	//
	// When `section_title` is NULL, this will report the lump sum parameter usage
	// for the entire run. When `section_title` is NOT NULL, this will only report
	// the parameters that were actually used (R/W) during the last section of the
	// run, i.e. since the previous invocation of this reporting method (or when
	// it hasn't been called before: the start of the application).
	//
	// Unless `f` is stdout/stderr, this method reports via `tprintf()` as well.
	// When `f` is a valid handle, then the report is written to the given FILE,
	// which may be stdout/stderr.
	//
	// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
	void ParamUtils::ReportParamsUsageStatistics(ParamsReportWriter &dst, const ParamsVectorSet &set, bool is_section_subreport, bool report_unused_params, const char *section_title) {
		if (!section_title || !*section_title)
			section_title = ParamUtils::GetApplicationName().c_str();

			dst.WriteReportHeaderLine(fmt::format("# {}: Parameter Usage Statistics: which params have been relevant?", section_title));

			// first collect all parameters and sort them:
			for (ParamsVector *vec : set.get()) {
				LIBASSERT_DEBUG_ASSERT(vec != nullptr);

				dst.WriteReportHeaderLine(fmt::format("## {}", vec->title()));

				// sort the parameters by name, per vectorset / section:
				std::vector<ParamPtr> params = vec->as_list();
				std::sort(params.begin(), params.end(), [](const ParamPtr& a, const ParamPtr& b)
				{
						int rv = sign_of_diff(a->is_init(), b->is_init());
						if (rv == 0)
						{
							rv = sign_of_diff(b->is_debug(), a->is_debug());
							if (rv == 0)
							{
								rv = strcmp(b->name_str(), a->name_str());
#if !defined(NDEBUG)
								if (rv == 0)
								{
									LIBASSERT_PANIC(fmt::format("Apparently you have double-defined a {} Variable: '{}'! Fix that in the source code!\n", ParamUtils::GetApplicationName(), a->name_str()).c_str());
								}
#endif
							}
						}
						return (rv >= 0);
				});

			static const char* categories[] = {"(Global)", "(Local)"};
			static const char* sections[] = {"", "(Init)", "(Debug)", "(Init+Dbg)"};
			static const char* write_access[] = {".", "w", "W"};
			static const char* read_access[] = {".", "r", "R"};

			if (!is_section_subreport) {
				// produce the final lump-sum overview report

				int count = 0;
				for (ParamPtr p : params) {
					p->reset_access_counts();

					auto stats = p->access_counts();
					if (stats.prev_sum_reading > 0)
					{
						count++;
					}
				}

				for (ParamPtr p : params) {
					auto stats = p->access_counts();
					if (stats.prev_sum_reading > 0)
					{
						int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
						std::string write_msg = fmt::format("{}{:4}", write_access[acc(stats.prev_sum_writing)], clip(stats.prev_sum_writing));
						if (acc(stats.prev_sum_writing) == 0)
							write_msg = ".    ";
						std::string read_msg = fmt::format("{}{:4}", read_access[acc(stats.prev_sum_reading)], clip(stats.prev_sum_reading));
						if (acc(stats.prev_sum_reading) == 0)
							read_msg = ".    ";
						std::string msg = fmt::format("* {:.<60} {:10} {}{} {:10} = {}\n", p->name_str(), sections[section], write_msg, read_msg, type_as_str(p->type()), p->formatted_value_str()));

						dst.WriteReportInfoLine(msg);
					}
				}

				if (report_all_variables)
				{
					writer->Write("\n\nUnused parameters:\n\n");

					for (ParamInfo &item : params) {
						const Param* p = item.p;
						auto stats = p->access_counts();
						if (stats.prev_sum_reading <= 0)
						{
							int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
							writer->Write(fmt::format("* {:.<60} {:8}{:10} {}{} {:9} = {}\n", p->name_str(), categories[item.global], sections[section], write_access[acc(stats.prev_sum_writing)], read_access[acc(stats.prev_sum_reading)], type_as_str(p->type()), p->formatted_value_str()));
						}
					}
				}
			} else {
				// produce the section-local report of used parameters

				for (ParamInfo &item : params) {
					const Param* p = item.p;
					auto stats = p->access_counts();
					if (stats.reading > 0)
					{
						int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
						writer->Write(fmt::format("* {:.<60} {:8}{:10} {}{} {:9} = {}\n", p->name_str(), categories[item.global], sections[section], write_access[acc(stats.prev_sum_writing)], read_access[acc(stats.prev_sum_reading)], type_as_str(p->type()), p->formatted_value_str()));
					}
				}

				// reset the access counts for the next section:
				for (ParamInfo &item : params) {
					Param* p = item.p;
					p->reset_access_counts();
				}
			}
		}

	}


	void ParamUtils::ResetToDefaults(const ParamsVectorSet& set, ParamSetBySourceType source_type) {

	}




	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamsReportWriter, et al
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ParamsReportDefaultWriter: public ParamsReportWriter {
	public:
		ParamsReportDefaultWriter(): ParamsReportWriter(nullptr) {}
		virtual ~ParamsReportDefaultWriter() = default;

		virtual void Write(const std::string message) {
			tprintDebug("{}", message);
		}

	protected:
	};

	class ParamsReportFileDuoWriter: public ParamsReportWriter {
	public:
		ParamsReportFileDuoWriter(FILE *f): ParamsReportWriter(f) {
			is_separate_file_ = (f != nullptr && f != stderr && f != stdout);
		}
		virtual ~ParamsReportFileDuoWriter() = default;

		virtual void Write(const std::string message) {
			// only write via tprintDebug() -- which usually logs to stderr -- when the `f` file destination is an actual file, rather than stderr or stdout.
			// This prevents these report lines showing up in duplicate on the console.
			if (is_separate_file_) {
				tprintDebug("{}", message);
			}
			size_t len = message.length();
			if (fwrite(message.c_str(), 1, len, file_) != len) {
				tprintError("Failed to write params-report line to file. {}\n", strerror(errno));
			}
		}

	protected:
		bool is_separate_file_;
	};





	

}	// namespace
