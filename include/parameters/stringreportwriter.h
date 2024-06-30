
#pragma once

#ifndef _LIB_PARAMS_STRINGREPORTWRITER_H_
#define _LIB_PARAMS_STRINGREPORTWRITER_H_

#include <parameters/reportwriter.h>
#include <parameters/CString.hpp>

#include <cstdint>
#include <string>

namespace parameters {

	// --------------------------------------------------------------------------------------------------

	class StringReportWriter: public ReportWriter {
	public:
		StringReportWriter(ReportType target = PARAMREPORT_AS_MARKDOWN_REPORT);
		StringReportWriter(reformatLine_f *custom_postprocessor, ReportType target = PARAMREPORT_CUSTOM_OUTPUT);
		virtual ~StringReportWriter();

		std::string to_string() const;

		CString<> &data();
	};

}

#endif
