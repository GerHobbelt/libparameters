/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_UTILS_H_
#define _LIB_PARAMS_UTILS_H_

#include <parameters/parameters.h>
#include <parameters/parameter_sets.h>

#include <cstdint>
#include <string>
#include <vector>


namespace parameters {

	class ConfigFile;
	class ReportFile;

#include <parameters/sourceref_defstart.h>

	// --------------------------------------------------------------------------------------------------

	// ParamsReportWriter, et al, used as support classes for the paramaeter usage reporting APIs ReportParamsUsageStatistics() et al.

	// The idea of the ParamsReportWriter class hierarchy is this:
	// 
	// We can output to one or more targets, each of arbitrary type, e.g. stdout (logging), config file, logfile.
	// 
	// The parameter info text is also printed, before the parameter value, and should be treated as comment.
	// When printing to stdout info text is included; the reportwriter must buffer and re-order the name + info=comment + value as desired..
	// Info text is omitted when printing to a basic config file.

	class ParamsReportWriter {
	public:
		ParamsReportWriter() {}
		virtual ~ParamsReportWriter() = default;

		virtual void WriteParamInfo(const Param &param) = 0;
		virtual void WriteParamValue(const Param &param) = 0;
		virtual void WriteReportHeaderLine(const std::string &message) = 0;
		virtual void WriteReportInfoLine(const std::string &message) = 0;
		virtual void WriteInfoLine(const std::string &message) = 0;
	};

	// --------------------------------------------------------------------------------------------------

	// Utility functions for working with Tesseract parameters.
	class ParamUtils {
	public:
		// Reads a file of parameter definitions and set/modify the values therein.
		// If the filename begins with a `+` or `-`, the Variables will be
		// ORed or ANDed with any current values.
		//
		// Blank lines and lines beginning # are ignored.
		//
		// Variable names are followed by one of more whitespace characters,
		// followed by the Value, which spans the rest of line.
		static bool ReadParamsFile(const std::string &file, // filename to read
															 const ParamsVectorSet &set,
															 SurplusParamsVector *surplus,
															 SOURCE_REF);

		// Read parameters from the given file pointer.
		// Otherwise identical to ReadParamsFile().
		static bool ReadParamsFile(ConfigFile &fp,
																 const ParamsVectorSet &set,
															 SurplusParamsVector *surplus,
																 SOURCE_REF);

		/**
		 * The default application source_type starts out as PARAM_VALUE_IS_SET_BY_ASSIGN.
		 * Discerning applications may want to set the default source type to PARAM_VALUE_IS_SET_BY_APPLICATION
		 * or PARAM_VALUE_IS_SET_BY_CONFIGFILE, depending on where the main workflow is currently at,
		 * while the major OCR tesseract APIs will set source type to PARAM_VALUE_IS_SET_BY_CORE_RUN
		 * (if the larger, embedding, application hasn't already).
		 *
		 * The purpose here is to be able to provide improved diagnostics reports about *who* did *what* to
		 * *which* parameters *when* exactly.
		 */
		static void set_current_application_default_param_source_type(ParamSetBySourceType source_type);

		/**
		 * Produces the current default application source type; intended to be used internally by our parameters support library code.
		 */
		static ParamSetBySourceType get_current_application_default_param_source_type();

		// Set the application name to be mentioned in libparameters' error messages.
		static void SetApplicationName(const char *appname = nullptr);
		static const std::string &GetApplicationName();

		// Set a parameter to have the given value.
		template <ParamAcceptableValueType T>
		static bool SetParam(
				const char *name, const T value,
				const ParamsVectorSet &set,
				SOURCE_REF);
		static bool SetParam(
				const char *name, const char *value,
				const ParamsVectorSet &set,
				SOURCE_REF);
		static bool SetParam(
				const char *name, const std::string &value,
				const ParamsVectorSet &set,
				SOURCE_REF);

		// Set a parameter to have the given value.
		template <ParamAcceptableValueType T>
		static bool SetParam(
				const char *name, const T value,
				ParamsVector &set,
				SOURCE_REF);
		static bool SetParam(
				const char *name, const char *value,
				ParamsVector &set,
				SOURCE_REF);
		static bool SetParam(
				const char *name, const std::string &value,
				ParamsVector &set,
				SOURCE_REF);

		// Produces a pointer (reference) to the parameter with the given name (of the
		// appropriate type) if it was found in any of the given vectors.
		// When `set` is empty, the `GlobalParams()` vector will be assumed
		// instead.
		//
		// Returns nullptr when param is not known.
		template <ParamDerivativeType T>
		static T *FindParam(
				const char *name,
				const ParamsVectorSet &set);
		static Param *FindParam(
				const char *name,
				const ParamsVectorSet &set,
				ParamType accepted_types_mask = ANY_TYPE_PARAM);

		// Produces a pointer (reference) to the parameter with the given name (of the
		// appropriate type) if it was found in any of the given vectors.
		// When `set` is empty, the `GlobalParams()` vector will be assumed
		// instead.
		//
		// Returns nullptr when param is not known.
		template <ParamDerivativeType T>
		static T *FindParam(
				const char *name,
				const ParamsVector &set);
		static Param *FindParam(
				const char *name,
				const ParamsVector &set,
				ParamType accepted_types_mask = ANY_TYPE_PARAM);

		// --------------------------------------------------------------------------------------------------

		// Print all parameters in the given set(s) to the given output.
		static void PrintParams(ParamsReportWriter &dst, const ParamsVectorSet &set, const char *section_title = nullptr);

		// Report parameters' usage statistics, i.e. report which params have been
		// set, modified and read/checked until now during this run-time's lifetime.
		//
		// Use this method for run-time 'discovery' about which tesseract parameters
		// are actually *used* during your particular usage of the library, ergo
		// answering the question:
		// "Which of all those parameters are actually *relevant* to my use case today?"
		//
		// When `is_section_subreport` is FALSE, this will report the lump sum parameter usage
		// for the entire run. When `is_section_subreport` is TRUE, this will only report
		// the parameters that were actually used (R/W) during the last section of the
		// run, i.e. since the previous invocation of this reporting method (or when
		// it hasn't been called before: the start of the application).
		//
		// Unless `f` is stdout/stderr, this method reports via `tprintf()` as well.
		// When `f` is a valid handle, then the report is written to the given FILE,
		// which may be stdout/stderr.
		//
		// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
		static void ReportParamsUsageStatistics(ParamsReportWriter &dst, const ParamsVectorSet &set, bool is_section_subreport, bool report_unused_params, const char *section_title = nullptr);

		// --------------------------------------------------------------------------------------------------

		// Resets all parameters back to default values;
		static void ResetToDefaults(const ParamsVectorSet &set, SOURCE_TYPE);
	};

	// template instances:
	template <>
	IntParam *ParamUtils::FindParam<IntParam>(
			const char *name,
			const ParamsVectorSet &set);
	template <>
	BoolParam *ParamUtils::FindParam<BoolParam>(
			const char *name,
			const ParamsVectorSet &set);
	template <>
	DoubleParam *ParamUtils::FindParam<DoubleParam>(
			const char *name,
			const ParamsVectorSet &set);
	template <>
	StringParam *ParamUtils::FindParam<StringParam>(
			const char *name,
			const ParamsVectorSet &set);
	template <>
	Param *ParamUtils::FindParam<Param>(
			const char *name,
			const ParamsVectorSet &set);
	template <ParamDerivativeType T>
	T *ParamUtils::FindParam(
			const char *name,
			const ParamsVector &set) {
		ParamsVectorSet pvec({const_cast<ParamsVector *>(&set)});

		return FindParam<T>(
				name,
				pvec);
	}
	template <>
	bool ParamUtils::SetParam<int32_t>(
			const char *name, const int32_t value,
			const ParamsVectorSet &set,
			ParamSetBySourceType source_type, ParamPtr source);
	template <>
	bool ParamUtils::SetParam<bool>(
			const char *name, const bool value,
			const ParamsVectorSet &set,
			ParamSetBySourceType source_type, ParamPtr source);
	template <>
	bool ParamUtils::SetParam<double>(
			const char *name, const double value,
			const ParamsVectorSet &set,
			ParamSetBySourceType source_type, ParamPtr source);
	template <ParamAcceptableValueType T>
	bool ParamUtils::SetParam(
			const char *name, const T value,
			ParamsVector &set,
			ParamSetBySourceType source_type, ParamPtr source) {
		ParamsVectorSet pvec({&set});
		return SetParam<T>(name, value, pvec, source_type, source);
	}

	// --------------------------------------------------------------------------------------------------

	// ready-made template instances:
	template <>
	IntParam *ParamsVectorSet::find<IntParam>(
			const char *name) const;
	template <>
	BoolParam *ParamsVectorSet::find<BoolParam>(
			const char *name) const;
	template <>
	DoubleParam *ParamsVectorSet::find<DoubleParam>(
			const char *name) const;
	template <>
	StringParam *ParamsVectorSet::find<StringParam>(
			const char *name) const;
	template <>
	Param *ParamsVectorSet::find<Param>(
			const char *name) const;

	// --------------------------------------------------------------------------------------------------

	// remove the macros to help set up the member prototypes

#include <parameters/sourceref_defend.h>

} // namespace 

#endif
