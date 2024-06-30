
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ReportFile
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// predefined formatters for the two predefined report types:
	//
	//		PARAMREPORT_AS_MARKDOWN_REPORT
	//		PARAMREPORT_AS_CONFIGFILE
	//
	// We also provide a bare bones formatter for the custom userland format (PARAMREPORT_CUSTOM_OUTPUT), in case the userland code forgets to specify one.
	//
	static void markdown_report_reformatLine(CString<> &line_buffer, size_t line_start_pos, ReportWriter::ReportType target, ReportWriter::LineContentPurpose purpose, int level /* starts at level 1 */, const Param *param) {

	}

	static void configfile_reformatLine(CString<> &line_buffer, size_t line_start_pos, ReportWriter::ReportType target, ReportWriter::LineContentPurpose purpose, int level /* starts at level 1 */, const Param *param) {

	}

	static void dummy_userland_report_reformatLine(CString<> &line_buffer, size_t line_start_pos, ReportWriter::ReportType target, ReportWriter::LineContentPurpose purpose, int level /* starts at level 1 */, const Param *param) {

	}

	static inline ReportWriter::reformatLine_f *pick_default_formatter_for_given_target(ReportWriter::ReportType target) {
		switch (target) {
		default:
		case ReportWriter::PARAMREPORT_CUSTOM_OUTPUT:
			return dummy_userland_report_reformatLine;

		case ReportWriter::PARAMREPORT_AS_MARKDOWN_REPORT:
			return markdown_report_reformatLine;

		case ReportWriter::PARAMREPORT_AS_CONFIGFILE:
			return configfile_reformatLine;
		}
	}

	ReportWriter::ReportWriter(ReportType target)
		: _postprocessor(pick_default_formatter_for_given_target(target))
		, _active_level(0)
		, _type(target)
	{}

	ReportWriter::ReportWriter(reformatLine_f *custom_postprocessor, ReportType target)
		: _postprocessor(custom_postprocessor != nullptr ? custom_postprocessor : pick_default_formatter_for_given_target(target))
		, _active_level(0)
		, _type(target)
	{}

	ReportWriter::~ReportWriter() {
		Finalize();
	}

	void ReportWriter::SetPostprocessor(reformatLine_f *custom_postprocessor) {
		_postprocessor = (custom_postprocessor != nullptr ? custom_postprocessor : pick_default_formatter_for_given_target(_type));
	}



	void ReportWriter::WriteParamInfoLine(const Param &param, ParamInfoElement show_elements) {
		size_t start_pos = _buffer.get_current_shift();
		switch (int(show_elements)) {
		case 0:
			break;

		case PARAMINFO_IDENTIFIER:
			_buffer.append(param.name_str());
			break;

		case PARAMINFO_TYPE_4_DISPLAY:
			_buffer.append(param.value_type_str());
			break;

		case PARAMINFO_VALUE_4_DISPLAY:
			_buffer.append(param.formatted_value_str());
			break;

		case PARAMINFO_DEFAULT_VALUE_4_DISPLAY:
			_buffer.append(param.formatted_default_value_str());
			break;

		case PARAMINFO_TYPE_4_INSPECT:
			_buffer.append(param.raw_value_type_str());
			break;

		case PARAMINFO_VALUE_4_INSPECT:
			_buffer.append(param.raw_value_str());
			break;

		case PARAMINFO_DEFAULT_VALUE_4_INSPECT:
			_buffer.append(param.raw_default_value_str());
			break;

		case PARAMINFO_STATUS_ATTRIBUTES:
			_buffer.append(param.name_str());
			_postprocessor(_buffer, start_pos, _type, PARAMREPORT_PARAM_ACTIVITY_ANALYSIS, _active_level, nullptr);
			break;

		case PARAMINFO_DESCRIPTION:
			_buffer.append(param.info_str());
			break;

			// usual combos:
		case PARAMINFO_DEFAULT_CONFIGFILE_LINE: //= PARAMINFO_IDENTIFIER | PARAMINFO_VALUE
			_buffer.append(param.info_str());
			break;
		case PARAMINFO_EXPLANATORY_CONFIGFILE_LINE: // PARAMINFO_IDENTIFIER | PARAMINFO_TYPE | PARAMINFO_VALUE | PARAMINFO_DESCRIPTION
			_buffer.append(param.info_str());
			break;
		case PARAMINFO_DEFAULT_STATUSREPORT_LINE: // PARAMINFO_IDENTIFIER | PARAMINFO_TYPE | PARAMINFO_VALUE | PARAMINFO_STATUS_ATTRIBUTES
			_buffer.append(param.info_str());
			_postprocessor(_buffer, start_pos, _type, PARAMREPORT_PARAM_ACTIVITY_ANALYSIS, _active_level, nullptr);
			break;
		case PARAMINFO_EXPLANATORY_STATUSREPORT_LINE: // PARAMINFO_IDENTIFIER | PARAMINFO_TYPE | PARAMINFO_VALUE | PARAMINFO_STATUS_ATTRIBUTES | PARAMINFO_DESCRIPTION
			_buffer.append(param.info_str());
			_postprocessor(_buffer, start_pos, _type, PARAMREPORT_PARAM_ACTIVITY_ANALYSIS, _active_level, nullptr);
			break;

		default:
			for (int element = PARAMINFO_DESCRIPTION; element != 0; element >>= 1) {
				switch (show_elements & element) {
				default:
				case 0:
					break;

				case PARAMINFO_IDENTIFIER:
					_buffer.append(param.name_str());
					break;

				case PARAMINFO_TYPE_4_DISPLAY:
					_buffer.append(param.value_type_str());
					break;

				case PARAMINFO_VALUE_4_DISPLAY:
					_buffer.append(param.formatted_value_str());
					break;

				case PARAMINFO_DEFAULT_VALUE_4_DISPLAY:
					_buffer.append(param.formatted_default_value_str());
					break;

				case PARAMINFO_TYPE_4_INSPECT:
					_buffer.append(param.raw_value_type_str());
					break;

				case PARAMINFO_VALUE_4_INSPECT:
					_buffer.append(param.raw_value_str());
					break;

				case PARAMINFO_DEFAULT_VALUE_4_INSPECT:
					_buffer.append(param.raw_default_value_str());
					break;

				case PARAMINFO_STATUS_ATTRIBUTES:
					_buffer.append(param.name_str());
					_postprocessor(_buffer, start_pos, _type, PARAMREPORT_PARAM_ACTIVITY_ANALYSIS, _active_level, nullptr);
					break;

				case PARAMINFO_DESCRIPTION:
					_buffer.append(param.info_str());
					break;
				}
			}
			break;
		}

		_postprocessor(_buffer, start_pos, _type, PARAMREPORT_PARAM_DECLARATION, _active_level, nullptr);
	}

	void ReportWriter::WriteHeaderLine(const std::string &message, int level /* starts at level 1 */) {
		_active_level = level;
		size_t start_pos = _buffer.get_current_shift();

		for (;level > 0; level--) {
			_buffer.append('#');
		}
		_buffer.append(' ');
		_buffer.append(message);

		_postprocessor(_buffer, start_pos, _type, PARAMREPORT_HEADING, _active_level, nullptr);
	}

	void ReportWriter::WriteInfoParagraph(const std::string &message) {
		size_t start_pos = _buffer.get_current_shift();
		_buffer.append(message);

		_postprocessor(_buffer, start_pos, _type, PARAMREPORT_INFO_PARAGRAPH, _active_level, nullptr);
	}

	void ReportWriter::WriteLineBuffer() {
		// nada. zilch.
	}

	void ReportWriter::Finalize() {
		// nada. zilch.

		_postprocessor(_buffer, 0, _type, PARAMREPORT_TERMINATION_AKA_THE_END, _active_level, nullptr);
	}





	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// StringReportWriter
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////


	StringReportWriter::StringReportWriter(ReportWriter::ReportType target)
		: ReportWriter(target)
	{}
	StringReportWriter::StringReportWriter(ReportWriter::reformatLine_f *custom_postprocessor, ReportWriter::ReportType target)
		: ReportWriter(custom_postprocessor, target)
	{}
	StringReportWriter::~StringReportWriter() = default;

	std::string StringReportWriter::to_string() const {
		return _buffer.c_str();
	}


	CString<> &StringReportWriter::data() {
		return _buffer;
	}





	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// StdioReportWriter
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// permanent lookup table:
	std::vector<std::string> StdioReportWriter::_processed_file_paths;


	StdioReportWriter::StdioReportWriter(const char *path, ReportType target)
		: StdioReportWriter(path, nullptr, target)
	{}
	StdioReportWriter::StdioReportWriter(const std::string &path, ReportType target)
		: StdioReportWriter(path.c_str(), nullptr, target)
	{}
	StdioReportWriter::StdioReportWriter(const std::string &path, reformatLine_f *custom_postprocessor, ReportType target)
		: StdioReportWriter(path.c_str(), custom_postprocessor, target)
	{}
	StdioReportWriter::StdioReportWriter(const char *path, reformatLine_f *custom_postprocessor, ReportType target)
		: ReportWriter(custom_postprocessor, target)
		, _f(nullptr)
		, _errored(false)
	{
		if (!path || !*path) {
			_f = nullptr;
			return;
		}

		if (strieq(path, "/dev/stdout") || strieq(path, "stdout") || strieq(path, "-") || strieq(path, "1")) {
			_f = stdout;
			_canonical_filepath = "/dev/stdout";
		} else if (strieq(path, "/dev/stderr") || strieq(path, "stderr") || strieq(path, "+") || strieq(path, "2")) {
			_f = stderr;
			_canonical_filepath = "/dev/stderr";
		} else {
			bool first = true;
			fs::path p = fs::weakly_canonical(path);
			std::u8string p8 = p.u8string();
			_canonical_filepath = reinterpret_cast<const char *>(p8.c_str());
			for (std::string &i : _processed_file_paths) {
				if (strieq(i.c_str(), _canonical_filepath.c_str())) {
					first = false;
					break;
				}
			}
			const char *mode = (first ? "w" : "a");
			_f = fopen(_canonical_filepath.c_str(), mode);
			if (!_f) {
				PARAM_ERROR("Cannot produce report/output file: {}\n", _canonical_filepath);
				_errored = true;
			} else if (first) {
				_processed_file_paths.push_back(_canonical_filepath.c_str());
			}
		}
	}


	StdioReportWriter::~StdioReportWriter() {
		Finalize();
	}

	FILE * StdioReportWriter::operator()() const {
		return _f;
	}


	void StdioReportWriter::WriteLineBuffer() {
		size_t sl = _buffer.length();
		size_t w = fwrite(_buffer.c_str(), 1, sl, _f);
		if (w != sl) {
			_errored = true;
			if (ferror(_f)) {
				PARAM_ERROR("Failed to write to file '{}': {}\n", _canonical_filepath, strerror(errno));
			} else {
				PARAM_ERROR("Failed to write to file '{}': unidentified error (disk full?)\n", _canonical_filepath);
			}
		}
		_buffer.clear();
	}


	void StdioReportWriter::Finalize() {
		if (!_errored) {
			if (!_buffer.empty()) {
				WriteLineBuffer();
			}
		}

		if (_f) {
			clearerr(_f);
			if (_f != stdout && _f != stderr) {
				fclose(_f);
			} else {
				fflush(_f);
			}
			if (ferror(_f)) {
				PARAM_ERROR("Failed to complete writing to file '{}': {}\n", _canonical_filepath, strerror(errno));
			} else {
				PARAM_ERROR("Failed to complate writing to file '{}': unidentified error (disk full?)\n", _canonical_filepath);
			}
		}
		_f = nullptr;
	}





}	// namespace
