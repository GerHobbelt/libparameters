
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

	
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
			const ParamsVectorSet *vec = ((v == 0) ? GlobalParams() : member_params);
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
