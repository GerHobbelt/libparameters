
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

#include <parameters/sourceref_defstart.h>

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// DoubleParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	void DoubleParam_ParamOnModifyFunction(DoubleParam &target, const double old_value, double &new_value, const double default_value, ParamSetBySourceType source_type, ParamPtr optional_setter) {
		// nothing to do
		return;
	}

	void DoubleParam_ParamOnValidateFunction(DoubleParam &target, const double old_value, double &new_value, const double default_value, ParamSetBySourceType source_type) {
		// nothing to do
		return;
	}

	void DoubleParam_ParamOnParseFunction(DoubleParam &target, double &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type) {
		const char *vs = source_value_str.c_str();
		char *endptr = nullptr;
		// https://stackoverflow.com/questions/25315191/need-to-clean-up-errno-before-calling-function-then-checking-errno?rq=3
		clear_errno();
#if 01
		double val = NAN;
		std::istringstream stream(source_value_str);
		// Use "C" locale for reading double value.
		stream.imbue(std::locale::classic());
		stream >> val;
		auto ec = errno;
		auto spos = stream.tellg();
		endptr = (char *)vs + spos;
		bool good = (endptr != vs && ec == E_OK);
#else
		auto val = strtod(vs, &endptr);
		bool good = (endptr != nullptr && ec == E_OK);
#endif
		std::string errmsg;
		if (good) {
			// check to make sure the tail is legal: whitespace only.
			// This also takes care of utter parse failure (when not already signaled via `errno`) when strtol() returns 0 and sets `endptr == vs`.
			while (isspace(*endptr))
				endptr++;
			good = (*endptr == '\0');

			// check if our parsed value is out of legal range: we check the type conversion as that is faster than checking against [INT32_MIN, INT32_MAX].
			if (!is_legal_fpval(val) && ec == E_OK) {
				good = false;
				ec = ERANGE;
			}
		} else {
			// failed to parse value.
			if (!endptr)
				endptr = (char *)vs;
		}
		if (!good) {
			target.fault();
			if (ec != E_OK) {
				if (ec == ERANGE) {
					errmsg = fmt::format("the parser stopped and reported an floating point value overflow (ERANGE); we accept floating point values between {} and {}.", std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
				} else {
					errmsg = fmt::format("the parser stopped and reported \"{}\" (errno: {})", strerror(ec), ec);
				}
			} else if (endptr > vs) {
				errmsg = fmt::format("the parser stopped early: the tail end (\"{}\") of the value string remains", endptr);
			} else {
				errmsg = "the parser was unable to parse anything at all";
			}
			PARAM_ERROR("ERROR: error parsing {} parameter '{}' value (\"{}\") to {}; {}. The parameter value will not be adjusted: the preset value ({}) will be used instead.\n", ParamUtils::GetApplicationName(), target.name_str(), source_value_str, target.value_type_str(), errmsg, target.formatted_value_str());

			// This value parse handler thus decides to NOT have a value written; we therefore signal a fault state right now: these are (non-fatal) non-silent errors.
			//
			// CODING TIP:
			//
			// When writing your own parse handlers, when you encounter truly very minor recoverable mistakes, you may opt to have such very minor mistakes be *slient*
			// by writing a WARNING message instead of an ERROR-level one and *not* invoking fault() -- such *silent mistakes* will consequently also not be counted
			// in the parameter fault statistics!
			//
			// IFF you want such minor mistakes to be counted anyway, we suggest to invoke `fault(); reset_fault();` which has the side-effect of incrementing the
			// error statistic without having ending up with a signaled fault state for the given parameter.
			// Here, today, however, we want the parse error to be non-silent and follow the behaviour as stated in the error message above: by signaling the fault state
			// before we leave, the remainder of this parameter write attempt will be aborted/skipped, as stated above.
			target.fault();
			// target.reset_fault();    -- commented out; here only as part of the CODING TIP above.

			// Finally, we should set the "parsed value" (`new_value`) to a sane value, despite our failure to parse the incoming number.
			// Hence we produce the previously value as that is the best sane value we currently know; the default value being the other option for this choice.
			new_value = target.value();
		} else {
			new_value = val;
		}
		pos = endptr - vs;
	}

	std::string DoubleParam_ParamOnFormatFunction(const DoubleParam &source, const double value, const double default_value, Param::ValueFetchPurpose purpose) {
		switch (purpose) {
			// Fetches the (raw, parseble for re-use via set_value()) value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_RAW_DATA_4_INSPECT:
			// Fetches the (formatted for print/display) value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DATA_FORMATTED_4_DISPLAY:
			// Fetches the (raw, parseble for re-use via set_value() or storing to serialized text data format files) value of the param as a string.
			//
			// NOTE: The part where the documentation says this variant MUST update the parameter usage statistics is
			// handled by the Param class code itself; no need for this callback to handle that part of the deal.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DATA_4_USE:
#if 0
			return std::to_string(value);   // always outputs %.6f format style values
#else
			char sbuf[40];
			snprintf(sbuf, sizeof(sbuf), "%1.f", value);
			sbuf[39] = 0;
			return sbuf;
#endif

			// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT:
			// Fetches the (formatted for print/display) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY:
#if 0
			return std::to_string(default_value);   // always outputs %.6f format style values
#else
			char sdbuf[40];
			snprintf(sdbuf, sizeof(sdbuf), "%1.f", default_value);
			sdbuf[39] = 0;
			return sdbuf;
#endif

			// Return string representing the type of the parameter value, e.g. "integer".
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_INSPECT:
			return "float";

		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY:
			return "floating point";

		default:
			DEBUG_ASSERT(0);
			return {};
		}
	}

	template<>
	DoubleParam::ValueTypedParam(const double value, THE_4_HANDLERS_PROTO_4_IMPL)
		: Param(name, comment, owner, init),
		on_modify_f_(on_modify_f ? on_modify_f : DoubleParam_ParamOnModifyFunction),
		on_validate_f_(on_validate_f ? on_validate_f : DoubleParam_ParamOnValidateFunction),
		on_parse_f_(on_parse_f ? on_parse_f : DoubleParam_ParamOnParseFunction),
		on_format_f_(on_format_f ? on_format_f : DoubleParam_ParamOnFormatFunction),
		value_(value),
		default_(value) {
		type_ = DOUBLE_PARAM;
	}

	template<>
	DoubleParam::operator double() const {
		return value();
	}

	template<>
	void DoubleParam::operator=(const double value) {
		set_value(value, ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}

	template<>
	void DoubleParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		unsigned int pos = 0;
		std::string vs(v);
		double vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, source_type); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			set_value(vv, source_type, source);
		}
	}

	template <>
	void DoubleParam::set_value(double value, ParamSetBySourceType source_type, ParamPtr source) {
		safe_inc(access_counts_.writing);
		// ^^^^^^^ --
		// Our 'writing' statistic counts write ATTEMPTS, in reailty.
		// Any real change is tracked by the 'changing' statistic (see further below)!

		reset_fault();
		// when we fail the validation horribly, the validator will throw an exception and thus abort the (write) action.
		// non-fatal errors may be signaled, in which case the write operation is aborted/skipped, or not signaled (a.k.a. 'silent')
		// in which case the write operation proceeds as if nothing untoward happened inside on_validate_f.
		on_validate_f_(*this, value_, value, default_, source_type);
		if (!has_faulted()) {
			// however, when we failed the validation only in the sense of the value being adjusted/restricted by the validator,
			// then we must set the value as set by the validator anyway, so nothing changes in our workflow here.

			set_ = (source_type > PARAM_VALUE_IS_RESET);
			set_to_non_default_value_ = (value != default_);

			if (value != value_) {
				on_modify_f_(*this, value_, value, default_, source_type, source);
				if (!has_faulted() && value != value_) {
					safe_inc(access_counts_.changing);
					value_ = value;
				}
			}
		}
		// any signaled fault will be visible outside...
	}

	template <>
	double DoubleParam::value() const noexcept {
		safe_inc(access_counts_.reading);
		return value_;
	}

	// Optionally the `source_vec` can be used to source the value to reset the parameter to.
	// When no source vector is specified, or when the source vector does not specify this
	// particular parameter, then our value is reset to the default value which was
	// specified earlier in our constructor.
	template<>
	void DoubleParam::ResetToDefault(const ParamsVectorSet *source_vec, ParamSetBySourceType source_type) {
		if (source_vec != nullptr) {
			DoubleParam *source = source_vec->find<DoubleParam>(name_str());
			if (source != nullptr) {
				set_value(source->value(), PARAM_VALUE_IS_RESET, source);
				return;
			}
		}
		set_value(default_, PARAM_VALUE_IS_RESET, nullptr);
	}

	template<>
	std::string DoubleParam::value_str(ValueFetchPurpose purpose) const {
		if (purpose == VALSTR_PURPOSE_DATA_4_USE)
			safe_inc(access_counts_.reading);
		return on_format_f_(*this, value_, default_, purpose);
	}

	template<>
	DoubleParam::ParamOnModifyFunction DoubleParam::set_on_modify_handler(DoubleParam::ParamOnModifyFunction on_modify_f) {
		DoubleParam::ParamOnModifyFunction rv = on_modify_f_;
		if (!on_modify_f)
			on_modify_f = DoubleParam_ParamOnModifyFunction;
		on_modify_f_ = on_modify_f;
		return rv;
	}
	template<>
	void DoubleParam::clear_on_modify_handler() {
		on_modify_f_ = DoubleParam_ParamOnModifyFunction;
	}
	template<>
	DoubleParam::ParamOnValidateFunction DoubleParam::set_on_validate_handler(DoubleParam::ParamOnValidateFunction on_validate_f) {
		DoubleParam::ParamOnValidateFunction rv = on_validate_f_;
		if (!on_validate_f)
			on_validate_f = DoubleParam_ParamOnValidateFunction;
		on_validate_f_ = on_validate_f;
		return rv;
	}
	template<>
	void DoubleParam::clear_on_validate_handler() {
		on_validate_f_ = DoubleParam_ParamOnValidateFunction;
	}
	template<>
	DoubleParam::ParamOnParseFunction DoubleParam::set_on_parse_handler(DoubleParam::ParamOnParseFunction on_parse_f) {
		DoubleParam::ParamOnParseFunction rv = on_parse_f_;
		if (!on_parse_f)
			on_parse_f = DoubleParam_ParamOnParseFunction;
		on_parse_f_ = on_parse_f;
		return rv;
	}
	template<>
	void DoubleParam::clear_on_parse_handler() {
		on_parse_f_ = DoubleParam_ParamOnParseFunction;
	}
	template<>
	DoubleParam::ParamOnFormatFunction DoubleParam::set_on_format_handler(DoubleParam::ParamOnFormatFunction on_format_f) {
		DoubleParam::ParamOnFormatFunction rv = on_format_f_;
		if (!on_format_f)
			on_format_f = DoubleParam_ParamOnFormatFunction;
		on_format_f_ = on_format_f;
		return rv;
	}
	template<>
	void DoubleParam::clear_on_format_handler() {
		on_format_f_ = DoubleParam_ParamOnFormatFunction;
	}



#include <parameters/sourceref_defend.h>

} // namespace tesseract
