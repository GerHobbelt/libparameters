
#pragma once

#ifndef _LIB_PARAMS_STDIOREPORTFILE_H_
#define _LIB_PARAMS_STDIOREPORTFILE_H_

#include <parameters/reportwriter.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace parameters {

	// --------------------------------------------------------------------------------------------------

	// A simple FILE/stdio wrapper class which supports writing to stdout/stderr or regular file.
	//
	// The very first time we call this one during the current run, we CREATE/OVERWRITE the target file.
	// Iff we happen to invoke this call multiple times for the same target file, any subsequent call
	// will APPEND to the target file.
	//
	// NOTE: as we need to track the given file path, this will incur a very minor heap memory leak
	// as we won't ever release the memory allocated for that string in `_processed_file_paths`.
	class StdioReportWriter: public ReportWriter {
	public:
		// Parse '-', 'stdout' and '1' as STDIN, '+', 'stderr', and '2' as STDERR, or open a regular text file in UTF8 write mode.
		//
		// MAY return NULL, e.g. when path is empty. This is considered a legal/valid use & behaviour.
		// On the other hand, an error line is printed via `tprintf()` when the given path is non-empty and
		// turns out not to be valid.
		// Either way, a return value of NULL implies that default behaviour (output via `tprintf()`) is assumed.
		StdioReportWriter(const char *path, ReportType target = PARAMREPORT_AS_MARKDOWN_REPORT);
		StdioReportWriter(const std::string &path, ReportType target = PARAMREPORT_AS_MARKDOWN_REPORT);
		StdioReportWriter(const char *path, reformatLine_f *custom_postprocessor, ReportType target = PARAMREPORT_CUSTOM_OUTPUT);
		StdioReportWriter(const std::string &path, reformatLine_f *custom_postprocessor, ReportType target = PARAMREPORT_CUSTOM_OUTPUT);

		virtual ~StdioReportWriter();

		FILE * operator()() const;

		operator bool() const {
			return _f != nullptr;
		};

	protected:
		virtual void WriteLineBuffer() override;

	public:
		virtual void Finalize() override;

	private:
		FILE *_f;
		std::string _canonical_filepath;
		bool _errored;

		// We assume we won't be processing very many file paths through here, so a linear scan through
		// the set-processed-to-date is considered good enough/best re performance, hence std::vector 
		// sufices for the tracking list.
		static std::vector<std::string> _processed_file_paths;
	};

}

#endif
