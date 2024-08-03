
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


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
	void ParamUtils::PrintParams(ReportWriter &dst, const ParamsVectorSet &set, ReportWriter::ParamInfoElement show_elements_style, const char *section_title) {
#if 0
		std::ostringstream stream;
		stream.imbue(std::locale::classic());
#endif
		if (!section_title || !*section_title)
			section_title = ParamUtils::GetApplicationName().c_str();

		dst.WriteHeaderLine(fmt::format("{} parameters overview", section_title), 1);

		for (ParamsVector *vec : set.get()) {
			LIBASSERT_DEBUG_ASSERT(vec != nullptr);

			dst.WriteHeaderLine(vec->title(), 2);

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
				dst.WriteParamInfoLine(param, show_elements_style);
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
	void ParamUtils::ReportParamsUsageStatistics(ReportWriter &dst, const ParamsVectorSet &set, int section_level, bool report_unused_params, ReportWriter::ParamInfoElement show_elements_style, const char *section_title) {
		if (!section_title || !*section_title)
			section_title = ParamUtils::GetApplicationName().c_str();

		// TODO
		bool is_section_subreport = (section_level <= 1);
		bool report_all_variables = report_unused_params;

		section_level = std::max(1, section_level + 1);

		dst.WriteHeaderLine(fmt::format("{}: Parameter Usage Statistics: which params have been relevant?", section_title), section_level);

		dst.WriteOther(ReportWriter::PARAMREPORT_TABLE_LEGENDA, "\n\n"
			"(WR legenda: `.`: zero/nil; `w`: written once, `W`: ~ twice or more; `r` = read once, `R`: ~ twice or more)\n"
			"\n\n");

		struct vectorInfo {
			std::string title;
			std::vector<ParamPtr> params;
		};
		std::vector<vectorInfo> info_set;

		// first collect all parameters and sort them:
		for (const ParamsVector *vec : set.get()) {
			LIBASSERT_DEBUG_ASSERT(vec != nullptr);

			vectorInfo &info = info_set.emplace_back(vec->title(), vec->as_list());

			// sort the parameters by name, per vectorset / section:
			std::sort(info.params.begin(), info.params.end(), [](const ParamPtr& a, const ParamPtr& b)
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
		}

		static const char* categories[] = {"(Global)", "(Local)"};
		static const char* sections[] = {"", "(Init)", "(Debug)", "(Init+Dbg)"};
		static const char* write_access[] = {".", "w", "W"};
		static const char* read_access[] = {".", "r", "R"};

		int total_count = 0;

		// then print the results collected thus far...
		for (vectorInfo &info : info_set) {
			std::string listmsg;
			int count = 0;

			if (!is_section_subreport) {
				// produce the final lump-sum overview report

				for (ParamPtr p : info.params) {
					p->reset_access_counts();
				}

				for (ParamPtr p : info.params) {
					auto stats = p->access_counts();
					if (stats.prev_sum_reading > 0)
					{
						count++;
						int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
						std::string write_msg = fmt::format("{}{:4}", write_access[acc(stats.prev_sum_writing)], clip(stats.prev_sum_writing));
						if (acc(stats.prev_sum_writing) == 0)
							write_msg = ".    ";
						std::string read_msg = fmt::format("{}{:4}", read_access[acc(stats.prev_sum_reading)], clip(stats.prev_sum_reading));
						if (acc(stats.prev_sum_reading) == 0)
							read_msg = ".    ";
						std::string msg = fmt::format("* {:.<60} {:10} {}{} {:10} = {}\n", p->name_str(), sections[section], write_msg, read_msg, type_as_str(p->type()), p->formatted_value_str());
						listmsg += msg;
					}
				}
			} else {
				// produce the section-local report of used parameters

				for (ParamPtr p : info.params) {
					auto stats = p->access_counts();
					if (stats.reading > 0)
					{
						count++;
						int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
						std::string write_msg = fmt::format("{}{:4}", write_access[acc(stats.writing)], clip(stats.writing));
						if (acc(stats.writing) == 0)
							write_msg = ".    ";
						std::string read_msg = fmt::format("{}{:4}", read_access[acc(stats.reading)], clip(stats.reading));
						if (acc(stats.reading) == 0)
							read_msg = ".    ";
						std::string msg = fmt::format("* {:.<60} {:10} {}{} {:10} = {}\n", p->name_str(), sections[section], write_msg, read_msg, type_as_str(p->type()), p->formatted_value_str());
						listmsg += msg;
					}
				}
			}

			if (count > 0) {
				total_count += count;

				dst.WriteHeaderLine(info.title, section_level + 1);

				dst.WriteOther(ReportWriter::PARAMREPORT_ITEM_LIST, listmsg);
			}
		}
		if (total_count == 0) {
			dst.WriteInfoParagraph("(No parameters were used in this run-time section.)\n");
		}
		int total_unused_count = 0;
		for (vectorInfo &info : info_set) {
			std::string listmsg;
			int count = 0;

			if (!is_section_subreport) {
				// produce the final lump-sum overview report

				for (ParamPtr p : info.params) {
					auto stats = p->access_counts();
					if (stats.prev_sum_reading <= 0)
					{
						count++;
						int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
						std::string write_msg = fmt::format("{}{:4}", write_access[acc(stats.prev_sum_writing)], clip(stats.prev_sum_writing));
						if (acc(stats.prev_sum_writing) == 0)
							write_msg = ".    ";
						const char *read_msg = ".    ";
						std::string msg = fmt::format("* {:.<60} {:10} {}{} {:10} = {}\n", p->name_str(), sections[section], write_msg, read_msg, type_as_str(p->type()), p->formatted_value_str());
						listmsg += msg;
					}
				}
			} else {
				// produce the section-local report of used parameters

				for (ParamPtr p : info.params) {
					auto stats = p->access_counts();
					if (stats.reading <= 0)
					{
						count++;
						int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
						std::string write_msg = fmt::format("{}{:4}", write_access[acc(stats.writing)], clip(stats.writing));
						if (acc(stats.writing) == 0)
							write_msg = ".    ";
						const char *read_msg = ".    ";
						std::string msg = fmt::format("* {:.<60} {:10} {}{} {:10} = {}\n", p->name_str(), sections[section], write_msg, read_msg, type_as_str(p->type()), p->formatted_value_str());
						listmsg += msg;
					}
				}
			}

			if (count > 0) {
				if (total_unused_count == 0) {
					dst.WriteHeaderLine("Unused Parameters:", section_level + 1);
				}
				total_unused_count += count;

				dst.WriteHeaderLine(info.title, section_level + 2);

				dst.WriteOther(ReportWriter::PARAMREPORT_ITEM_LIST, listmsg);
			}
		}
		// reset the access counts for the next section:
		for (vectorInfo &info : info_set) {
			for (ParamPtr p : info.params) {
				p->reset_access_counts();
			}
		}
	}


}	// namespace
