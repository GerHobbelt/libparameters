
#include <parameters/parameters.h>

#include "internal_helpers.hpp"


namespace parameters {

	void ParamUtils::PrintParams(FILE* fp, const ParamsVectorSet& set, bool print_info) {

	}

	void ParamUtils::ReportParamsUsageStatistics(FILE* fp, const ParamsVectorSet& set, const char* section_title) {

	}

	void ParamUtils::ResetToDefaults(const ParamsVectorSet& set, ParamSetBySourceType source_type) {

	}




	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamsReportWriter, et al
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ParamsReportWriter {
	public:
		ParamsReportWriter(FILE *f)
			: file_(f) {}
		virtual ~ParamsReportWriter() = default;

		virtual void Write(const std::string message) = 0;

	protected:
		FILE *file_;
	};

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
		case ANY_TYPE_PARAM:
			return "[ANY]";
		default:
			return "[???]";
		}
	}




	// When `section_title` is NULL, this will report the lump sum parameter usage for the entire run.
	// When `section_title` is NOT NULL, this will only report the parameters that were actually used (R/W) during the last section of the run, i.e.
	// since the previous invocation of this reporting method (or when it hasn't been called before: the start of the application).
	void ParamUtils::ReportParamsUsageStatistics(FILE *f, const ParamsVectorSet *member_params, const char *section_title)
	{
		bool is_section_subreport = (section_title != nullptr);

		std::unique_ptr<ParamsReportWriter> writer;

		if (f != nullptr) {
			writer.reset(new ParamsReportFileDuoWriter(f));
		} else {
			writer.reset(new ParamsReportDefaultWriter());
		}

		writer->Write(fmt::format("\n\n{} Parameter Usage Statistics{}: which params have been relevant?\n"
			"----------------------------------------------------------------------\n\n",
			ParamUtils::GetApplicationName(), (section_title != nullptr ? fmt::format(" for section: {}", section_title) : "")));

		// first collect all parameters and sort them according to these criteria:
		// - global / (class)local
		// - name

		const ParamsVector* globals = GlobalParams();

		struct ParamInfo {
			Param *p;
			bool global;
		};

		std::vector<ParamInfo> params;
		{
			std::vector<Param *> ll = globals->as_list();
			params.reserve(ll.size());
			for (Param *i : ll) {
				params.push_back({i, true});
			}
		}

		if (member_params != nullptr) {
			std::vector<Param *> ll = member_params->as_list();
			params.reserve(ll.size() + params.size());
			for (Param *i : ll) {
				params.push_back({i, false});
			}
		}

		std::sort(params.begin(), params.end(), [](ParamInfo& a, ParamInfo& b)
		{
			int rv = (int)a.global - (int)b.global;
			if (rv == 0)
			{
				rv = (int)a.p->is_init() - (int)b.p->is_init();
				if (rv == 0)
				{
					rv = (int)b.p->is_debug() - (int)a.p->is_debug();
					if (rv == 0)
					{
						rv = strcmp(b.p->name_str(), a.p->name_str());
#if !defined(NDEBUG)
						if (rv == 0)
						{
							fprintf(stderr, "Apparently you have double-defined {} Variable: '%s'! Fix that in the source code!\n", ParamUtils::GetApplicationName(), a.p->name_str());
							DEBUG_ASSERT(!"Apparently you have double-defined a Variable.");
						}
#endif
					}
				}
			}
			return (rv >= 0);
		});

		static const char* categories[] = {"(Global)", "(Local)"};
		static const char* sections[] = {"", "(Init)", "(Debug)", "(Init+Dbg)"};
		static const char* write_access[] = {".", "w", "W"};
		static const char* read_access[] = {".", "r", "R"};

		auto acc = [](int access) {
			if (access > 2)
				access = 2;
			return access;
			};

		if (!is_section_subreport) {
			// produce the final lump-sum overview report

			for (ParamInfo &item : params) {
				Param *p = item.p;
				p->reset_access_counts();
			}

			for (ParamInfo &item : params) {
				const Param* p = item.p;
				auto stats = p->access_counts();
				if (stats.prev_sum_reading > 0)
				{
					int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
					writer->Write(fmt::format("* {:.<60} {:8}{:10} {}{} {:9} = {}\n", p->name_str(), categories[item.global], sections[section], write_access[acc(stats.prev_sum_writing)], read_access[acc(stats.prev_sum_reading)], type_as_str(p->type()), p->formatted_value_str()));
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

	void ParamUtils::PrintParams(FILE *fp, const ParamsVectorSet *member_params, bool print_info) {
		int num_iterations = (member_params == nullptr) ? 1 : 2;
		// When printing to stdout info text is included.
		// Info text is omitted when printing to a file (would result in an invalid config file).
		if (!fp)
			fp = stdout;
		bool printing_to_stdio = (fp == stdout || fp == stderr);
		std::ostringstream stream;
		stream.imbue(std::locale::classic());
		for (int v = 0; v < num_iterations; ++v) {
			const ParamsVectorSet *vec = (v == 0) ? GlobalParams() : member_params;
			for (auto int_param : vec->int_params_c()) {
				if (print_info) {
					stream << int_param->name_str() << '\t' << (int32_t)(*int_param) << '\t'
						<< int_param->info_str() << '\n';
				} else {
					stream << int_param->name_str() << '\t' << (int32_t)(*int_param) << '\n';
				}
			}
			for (auto bool_param : vec->bool_params_c()) {
				if (print_info) {
					stream << bool_param->name_str() << '\t' << bool(*bool_param) << '\t'
						<< bool_param->info_str() << '\n';
				} else {
					stream << bool_param->name_str() << '\t' << bool(*bool_param) << '\n';
				}
			}
			for (auto string_param : vec->string_params_c()) {
				if (print_info) {
					stream << string_param->name_str() << '\t' << string_param->c_str() << '\t'
						<< string_param->info_str() << '\n';
				} else {
					stream << string_param->name_str() << '\t' << string_param->c_str() << '\n';
				}
			}
			for (auto double_param : vec->double_params_c()) {
				if (print_info) {
					stream << double_param->name_str() << '\t' << (double)(*double_param) << '\t'
						<< double_param->info_str() << '\n';
				} else {
					stream << double_param->name_str() << '\t' << (double)(*double_param) << '\n';
				}
			}
		}
		if (printing_to_stdio)
		{
			tprintDebug("{}", stream.str().c_str());
		} else
		{
			fprintf(fp, "%s", stream.str().c_str());
		}
	}

}	// namespace
