/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_REPORTWRITER_H_
#define _LIB_PARAMS_REPORTWRITER_H_

#include <parameters/parameter_classes.h>
#include <parameters/parameter_sets.h>
#include <parameters/CString.hpp>

#include <cstdint>


namespace parameters {

	// --------------------------------------------------------------------------------------------------

	// ReportWriter, et al, used as support classes for the paramaeter usage reporting APIs ReportParamsUsageStatistics() et al.

	// The idea of the ReportWriter class hierarchy is this:
	// 
	// We can output to one or more targets, each of arbitrary type, e.g. stdout (logging), config file, logfile.
	// 
	// The parameter info text is also printed, before the parameter value, and should be treated as comment.
	// When printing to stdout info text is included; the reportwriter must buffer and re-order the name + info=comment + value as desired..
	// Info text is omitted when printing to a basic config file.

	class ReportWriter {
	public:
		enum ReportType: int {
			PARAMREPORT_CUSTOM_OUTPUT = 0,
			PARAMREPORT_AS_MARKDOWN_REPORT = 1,
			PARAMREPORT_AS_CONFIGFILE,
		};
		enum LineContentPurpose: int {
			PARAMREPORT_TERMINATION_AKA_THE_END = 0,
			PARAMREPORT_HEADING,
			PARAMREPORT_INFO_PARAGRAPH,
			PARAMREPORT_PARAM_ACTIVITY_ANALYSIS,
			PARAMREPORT_PARAM_DECLARATION,
			PARAMREPORT_TABLE_LEGENDA,
			PARAMREPORT_ITEM_LIST,
		};

		// userdef function proto: may pass or rewrite the line_buffer, depending on target, purpose or other extraneous userland reasons.
		typedef void reformatLine_f(CString<> &line_buffer, size_t line_start_pos, ReportType target, LineContentPurpose purpose, int level /* starts at level 1 */, const Param *param);

	public:
		ReportWriter(ReportType target = PARAMREPORT_AS_MARKDOWN_REPORT);
		ReportWriter(reformatLine_f *custom_postprocessor, ReportType target = PARAMREPORT_CUSTOM_OUTPUT);
		virtual ~ReportWriter();

		void SetPostprocessor(reformatLine_f *custom_postprocessor = nullptr);

		enum ParamInfoElement: int {
			PARAMINFO_IDENTIFIER = 0x01,
			PARAMINFO_TYPE_4_INSPECT = 0x02,
			PARAMINFO_TYPE_4_DISPLAY = 0x04,
			PARAMINFO_VALUE_4_INSPECT = 0x08,
			PARAMINFO_VALUE_4_DISPLAY = 0x10,
			PARAMINFO_DEFAULT_VALUE_4_INSPECT = 0x20,
			PARAMINFO_DEFAULT_VALUE_4_DISPLAY = 0x40,
			PARAMINFO_STATUS_ATTRIBUTES = 0x80,
			PARAMINFO_DESCRIPTION = 0x100,

			// usual combos:
			PARAMINFO_DEFAULT_CONFIGFILE_LINE = PARAMINFO_IDENTIFIER | PARAMINFO_VALUE_4_INSPECT,
			PARAMINFO_EXPLANATORY_CONFIGFILE_LINE = PARAMINFO_IDENTIFIER | PARAMINFO_TYPE_4_INSPECT | PARAMINFO_VALUE_4_INSPECT | PARAMINFO_DESCRIPTION,
			PARAMINFO_DEFAULT_STATUSREPORT_LINE = PARAMINFO_IDENTIFIER | PARAMINFO_TYPE_4_DISPLAY | PARAMINFO_VALUE_4_DISPLAY | PARAMINFO_DEFAULT_VALUE_4_DISPLAY | PARAMINFO_STATUS_ATTRIBUTES,
			PARAMINFO_EXPLANATORY_STATUSREPORT_LINE = PARAMINFO_IDENTIFIER | PARAMINFO_TYPE_4_DISPLAY | PARAMINFO_VALUE_4_DISPLAY | PARAMINFO_DEFAULT_VALUE_4_DISPLAY | PARAMINFO_STATUS_ATTRIBUTES | PARAMINFO_DESCRIPTION,
		};

		void WriteParamInfoLine(const Param &param, ParamInfoElement show_elements);
		void WriteParamInfoLine(const Param *param, ParamInfoElement show_elements) {
			WriteParamInfoLine(*param, show_elements);
		}
		void WriteHeaderLine(const std::string &message, int level /* starts at level 1 */);
		void WriteInfoParagraph(const std::string &message);
		void WriteOther(LineContentPurpose purpose, const std::string &message);

	protected:
		virtual void WriteLineBuffer();

	public:
		virtual void Finalize();

	protected:
		reformatLine_f *_postprocessor;
		CString<> _buffer;
		int _active_level;
		ReportType _type;
	};

} // namespace 

#endif
