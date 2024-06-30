
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// IntParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	void IntParam_ParamOnModifyFunction(IntParam &target, const int32_t old_value, int32_t &new_value, const int32_t default_value, ParamSetBySourceType source_type, ParamPtr optional_setter) {
		// nothing to do
		return;
	}

	void IntParam_ParamOnValidateFunction(IntParam &target, const int32_t old_value, int32_t &new_value, const int32_t default_value, ParamSetBySourceType source_type) {
		// nothing to do
		return;
	}

	void IntParam_ParamOnParseFunction(IntParam &target, int32_t &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type) {
		const char *vs = source_value_str.c_str();
		char *endptr = nullptr;
		// https://stackoverflow.com/questions/25315191/need-to-clean-up-errno-before-calling-function-then-checking-errno?rq=3
#if defined(_MSC_VER)
		_set_errno(E_OK);
#else
		errno = E_OK;
#endif
		auto parsed_value = strtol(vs, &endptr, 10);
		auto ec = errno;
		int32_t val = int32_t(parsed_value);
		bool good = (endptr != nullptr && ec == E_OK);
		std::string errmsg;
		if (good) {
			// check to make sure the tail is legal: whitespace only.
			// This also takes care of utter parse failure (when not already signaled via `errno`) when strtol() returns 0 and sets `endptr == vs`.
			while (isspace(*endptr))
				endptr++;
			good = (*endptr == '\0');

			// check if our parsed value is out of legal range: we check the type conversion as that is faster than checking against [INT32_MIN, INT32_MAX].
			if (val != parsed_value && ec == E_OK) {
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
					errmsg = fmt::format("the parser stopped and reported an integer value overflow (ERANGE); we accept decimal values between {} and {}.", std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
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
			//target.reset_fault();    -- commented out; here only as part of the CODING TIP above.

			// Finally, we should set the "parsed value" (`new_value`) to a sane value, despite our failure to parse the incoming number.
			// Hence we produce the previously value as that is the best sane value we currently know; the default value being the other option for this choice.
			new_value = target.value();
		} else {
			new_value = val;
		}
		pos = endptr - vs;
	}

	std::string IntParam_ParamOnFormatFunction(const IntParam &source, const int32_t value, const int32_t default_value, Param::ValueFetchPurpose purpose) {
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
			return std::to_string(value);

			// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT:
			// Fetches the (formatted for print/display) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY:
			return std::to_string(default_value);

			// Return string representing the type of the parameter value, e.g. "integer".
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_INSPECT:
			return "int32_t";

		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY:
			return "integer";

		default:
			DEBUG_ASSERT(0);
			return nullptr;
		}
	}

	template<>
	IntParam::ValueTypedParam(const int32_t value, THE_4_HANDLERS_PROTO)
		: Param(name, comment, owner, init),
		on_modify_f_(on_modify_f ? on_modify_f : IntParam_ParamOnModifyFunction),
		on_validate_f_(on_validate_f ? on_validate_f : IntParam_ParamOnValidateFunction),
		on_parse_f_(on_parse_f ? on_parse_f : IntParam_ParamOnParseFunction),
		on_format_f_(on_format_f ? on_format_f : IntParam_ParamOnFormatFunction),
		value_(value),
		default_(value)
	{
		type_ = INT_PARAM;
	}

	template<>
	IntParam::operator int32_t() const {
		return value();
	}

	template<>
	void IntParam::operator=(const int32_t value) {
		set_value(value, ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}

	template<>
	void IntParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		unsigned int pos = 0;
		std::string vs(v);
		int32_t vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, source_type); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			set_value(vv, source_type, source);
		}
	}

	template<>
	void IntParam::set_value(int32_t value, ParamSetBySourceType source_type, ParamPtr source) {
		reset_fault();

		if (!can_update(source_type)) {
			PARAM_TRACE("Silently ignoring parameter {} write due to precedence: current level: {} vs. write attempt level: {} (present value: {}, skipped: {})\n",
				name_str(), set_mode(), source_type, formatted_value_str(), value);
			return;
		}

		safe_inc(access_counts_.writing);
		// ^^^^^^^ --
		// Our 'writing' statistic counts write ATTEMPTS, in reailty.
		// Any real change is tracked by the 'changing' statistic (see further below)!

		// when we fail the validation horribly, the validator will throw an exception and thus abort the (write) action.
		// non-fatal errors may be signaled, in which case the write operation is aborted/skipped, or not signaled (a.k.a. 'silent')
		// in which case the write operation proceeds as if nothing untoward happened inside on_validate_f.
		on_validate_f_(*this, value_, value, default_, source_type);
		if (!has_faulted()) {
			// however, when we failed the validation only in the sense of the value being adjusted/restricted by the validator,
			// then we must set the value as set by the validator anyway, so nothing changes in our workflow here.
			if (value != value_) {
				on_modify_f_(*this, value_, value, default_, source_type, source);
				if (!has_faulted()) {
					if (value != value_) {
						safe_inc(access_counts_.changing);
						value_ = value;

						set_to_non_default_value_ = (value != default_);
					}

					set_mode_ = source_type;
					setter_ = source;
					set_ = (source_type > PARAM_VALUE_IS_RESET);
				}
			}
		}
		// any signaled fault will be visible outside...
	}

	template <>
	int32_t IntParam::value() const noexcept {
		safe_inc(access_counts_.reading);
		return value_;
	}

	// Optionally the `source_vec` can be used to source the value to reset the parameter to.
	// When no source vector is specified, or when the source vector does not specify this
	// particular parameter, then our value is reset to the default value which was
	// specified earlier in our constructor.
	template<>
	void IntParam::ResetToDefault(const ParamsVectorSet *source_vec, ParamSetBySourceType source_type) {
		if (source_vec != nullptr) {
			IntParam *source = source_vec->find<IntParam>(name_str());
			if (source != nullptr) {
				set_value(source->value(), PARAM_VALUE_IS_RESET, source);
				return;
			}
		}
		set_value(default_, PARAM_VALUE_IS_RESET, nullptr);
	}

	template<>
	std::string IntParam::value_str(ValueFetchPurpose purpose) const {
		if (purpose == VALSTR_PURPOSE_DATA_4_USE)
			safe_inc(access_counts_.reading);
		return on_format_f_(*this, value_, default_, purpose);
	}

	template<>
	IntParam::ParamOnModifyFunction IntParam::set_on_modify_handler(IntParam::ParamOnModifyFunction on_modify_f) {
		IntParam::ParamOnModifyFunction rv = on_modify_f_;
		if (!on_modify_f)
			on_modify_f = IntParam_ParamOnModifyFunction;
		on_modify_f_ = on_modify_f;
		return rv;
	}
	template<>
	void IntParam::clear_on_modify_handler() {
		on_modify_f_ = IntParam_ParamOnModifyFunction;
	}
	template<>
	IntParam::ParamOnValidateFunction IntParam::set_on_validate_handler(IntParam::ParamOnValidateFunction on_validate_f) {
		IntParam::ParamOnValidateFunction rv = on_validate_f_;
		if (!on_validate_f)
			on_validate_f = IntParam_ParamOnValidateFunction;
		on_validate_f_ = on_validate_f;
		return rv;
	}
	template<>
	void IntParam::clear_on_validate_handler() {
		on_validate_f_ = IntParam_ParamOnValidateFunction;
	}
	template<>
	IntParam::ParamOnParseFunction IntParam::set_on_parse_handler(IntParam::ParamOnParseFunction on_parse_f) {
		IntParam::ParamOnParseFunction rv = on_parse_f_;
		if (!on_parse_f)
			on_parse_f = IntParam_ParamOnParseFunction;
		on_parse_f_ = on_parse_f;
		return rv;
	}
	template<>
	void IntParam::clear_on_parse_handler() {
		on_parse_f_ = IntParam_ParamOnParseFunction;
	}
	template<>
	IntParam::ParamOnFormatFunction IntParam::set_on_format_handler(IntParam::ParamOnFormatFunction on_format_f) {
		IntParam::ParamOnFormatFunction rv = on_format_f_;
		if (!on_format_f)
			on_format_f = IntParam_ParamOnFormatFunction;
		on_format_f_ = on_format_f;
		return rv;
	}
	template<>
	void IntParam::clear_on_format_handler() {
		on_format_f_ = IntParam_ParamOnFormatFunction;
	}

#if 0
	template<>
	bool IntParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		int32_t val = 0;
		return SafeAtoi(v, &val) && set_value(val, source_type, source);
	}
	template<>
	bool IntParam::set_value(bool v, ParamSetBySourceType source_type, ParamPtr source) {
		int32_t val = !!v;
		return set_value(val, source_type, source);
	}
	template<>
	bool IntParam::set_value(double v, ParamSetBySourceType source_type, ParamPtr source) {
		v = roundf(v);
		if (v < INT32_MIN || v > INT32_MAX)
			return false;

		int32_t val = v;
		return set_value(val, source_type, source);
	}
#endif




	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// BoolParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	void BoolParam_ParamOnModifyFunction(BoolParam &target, const bool old_value, bool &new_value, const bool default_value, ParamSetBySourceType source_type, ParamPtr optional_setter) {
		// nothing to do
		return;
	}

	void BoolParam_ParamOnValidateFunction(BoolParam &target, const bool old_value, bool &new_value, const bool default_value, ParamSetBySourceType source_type) {
		// nothing to do
		return;
	}

	void BoolParam_ParamOnParseFunction(BoolParam &target, bool &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type) {
		const char *vs = source_value_str.c_str();
		char *endptr = nullptr;
		// https://stackoverflow.com/questions/25315191/need-to-clean-up-errno-before-calling-function-then-checking-errno?rq=3
#if defined(_MSC_VER)
		_set_errno(E_OK);
#else
		errno = E_OK;
#endif
		// We accept decimal, hex and octal numbers here, not just the ubiquitous 0, 1 and -1. `+5` also implies TRUE as far as we are concerned. We are tolerant on our input here, not pedantic, *by design*.
		// However, we do restrict our values to the 32-bit signed range: this is picked for the tolerated numeric value range as that equals the IntPAram (int32_t) one, but granted: this range restriction is
		// a matter of taste and arguably arbitrary. We've pondered limiting the accepted numerical values to the range of an int8_t (-128 .. + 127) but the ulterior goal here is to stay as close to the int32_t
		// IntParam value parser code as possible, so int32_t range it is....
		auto parsed_value = strtol(vs, &endptr, 0);
		auto ec = errno;
		int32_t val = int32_t(parsed_value);
		bool good = (endptr != nullptr && ec == E_OK);
		std::string errmsg;
		if (good) {
			// check to make sure the tail is legal: whitespace only.
			// This also takes care of utter parse failure (when not already signaled via `errno`) when strtol() returns 0 and sets `endptr == vs`.
			while (isspace(*endptr))
				endptr++;
			good = (*endptr == '\0');

			// check if our parsed value is out of legal range: we check the type conversion as that is faster than checking against [INT32_MIN, INT32_MAX].
			if (val != parsed_value && ec == E_OK) {
				good = false;
				ec = ERANGE;
			}
		} else {
			// failed to parse boolean value as numeric value (zero, non-zero). Try to parse as a word (true/false) or symbol (+/-) instead.
			const char *s = vs;
			while (isspace(*s))
				s++;
			endptr = (char *)vs;
			switch (tolower(s[0])) {
			case 't':
				// true; only valid when a single char or word:
				// (and, yes, we are very lenient: if some Smart Alec enters "Tamagotchi" as a value here, we consider that a valid equivalent to TRUE. Tolerant *by design*.)
				good = is_single_word(s);
				val = 1;
				break;

			case 'f':
				// false; only valid when a single char or word:
				// (and, yes, we are very lenient again: if some Smart Alec enters "Favela" as a value here, we consider that a valid equivalent to FALSE. Tolerant *by design*. Bite me.)
				good = is_single_word(s);
				val = 0;
				break;

			case 'y':
			case 'j':
				// yes / ja; only valid when a single char or word:
				good = is_single_word(s);
				val = 1;
				break;

			case 'n':
				// no; only valid when a single char or word:
				good = is_single_word(s);
				val = 0;
				break;

			case 'x':
			case '+':
				// on; only valid when alone:
				good = is_optional_whitespace(s + 1);
				val = 1;
				break;

			case '-':
			case '.':
				// off; only valid when alone:
				good = is_optional_whitespace(s + 1);
				val = 0;
				break;

			default:
				// we reject everything else as not-a-boolean-value.
				good = false;
				break;
			}

			if (good) {
				endptr += strlen(endptr);
			}
		}

		if (!good) {
			target.fault();
			if (ec != E_OK) {
				if (ec == ERANGE) {
					errmsg = fmt::format("the parser stopped and reported an integer value overflow (ERANGE); while we expect a boolean value (ideally 1/0/-1), we accept decimal values between {} and {} where any non-zero value equals TRUE.", std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
				} else {
					errmsg = fmt::format("the parser stopped and reported \"{}\" (errno: {}) and we were unable to otherwise parse the given value as a boolean word ([T]rue/[F]alse/[Y]es/[J]a/[N]o) or boolean symbol (+/-/.) ", strerror(ec), ec);
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
		}
		pos = endptr - vs;
	}

	std::string BoolParam_ParamOnFormatFunction(const BoolParam &source, const bool value, const bool default_value, Param::ValueFetchPurpose purpose) {
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
			return value ? "true" : "false";

			// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT:
			// Fetches the (formatted for print/display) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY:
			return default_value ? "true" : "false";

			// Return string representing the type of the parameter value, e.g. "integer".
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_INSPECT:
			return "bool";

		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY:
			return "boolean";

		default:
			DEBUG_ASSERT(0);
			return nullptr;
		}
	}

	template<>
	BoolParam::ValueTypedParam(const bool value, THE_4_HANDLERS_PROTO)
		: Param(name, comment, owner, init),
		on_modify_f_(on_modify_f ? on_modify_f : BoolParam_ParamOnModifyFunction),
		on_validate_f_(on_validate_f ? on_validate_f : BoolParam_ParamOnValidateFunction),
		on_parse_f_(on_parse_f ? on_parse_f : BoolParam_ParamOnParseFunction),
		on_format_f_(on_format_f ? on_format_f : BoolParam_ParamOnFormatFunction),
		value_(value),
		default_(value) {
		type_ = BOOL_PARAM;
	}

	template<>
	BoolParam::operator bool() const {
		return value();
	}

	template<>
	void BoolParam::operator=(const bool value) {
		set_value(value, ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}

	template<>
	void BoolParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		unsigned int pos = 0;
		std::string vs(v);
		bool vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, source_type); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			set_value(vv, source_type, source);
		}
	}

	template <>
	void BoolParam::set_value(bool value, ParamSetBySourceType source_type, ParamPtr source) {
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
	bool BoolParam::value() const noexcept {
		safe_inc(access_counts_.reading);
		return value_;
	}

	// Optionally the `source_vec` can be used to source the value to reset the parameter to.
	// When no source vector is specified, or when the source vector does not specify this
	// particular parameter, then our value is reset to the default value which was
	// specified earlier in our constructor.
	template<>
	void BoolParam::ResetToDefault(const ParamsVectorSet *source_vec, ParamSetBySourceType source_type) {
		if (source_vec != nullptr) {
			BoolParam *source = source_vec->find<BoolParam>(name_str());
			if (source != nullptr) {
				set_value(source->value(), PARAM_VALUE_IS_RESET, source);
				return;
			}
		}
		set_value(default_, PARAM_VALUE_IS_RESET, nullptr);
	}

	template<>
	std::string BoolParam::value_str(ValueFetchPurpose purpose) const {
		if (purpose == VALSTR_PURPOSE_DATA_4_USE)
			safe_inc(access_counts_.reading);
		return on_format_f_(*this, value_, default_, purpose);
	}

	template<>
	BoolParam::ParamOnModifyFunction BoolParam::set_on_modify_handler(BoolParam::ParamOnModifyFunction on_modify_f) {
		BoolParam::ParamOnModifyFunction rv = on_modify_f_;
		if (!on_modify_f)
			on_modify_f = BoolParam_ParamOnModifyFunction;
		on_modify_f_ = on_modify_f;
		return rv;
	}
	template<>
	void BoolParam::clear_on_modify_handler() {
		on_modify_f_ = BoolParam_ParamOnModifyFunction;
	}
	template<>
	BoolParam::ParamOnValidateFunction BoolParam::set_on_validate_handler(BoolParam::ParamOnValidateFunction on_validate_f) {
		BoolParam::ParamOnValidateFunction rv = on_validate_f_;
		if (!on_validate_f)
			on_validate_f = BoolParam_ParamOnValidateFunction;
		on_validate_f_ = on_validate_f;
		return rv;
	}
	template<>
	void BoolParam::clear_on_validate_handler() {
		on_validate_f_ = BoolParam_ParamOnValidateFunction;
	}
	template<>
	BoolParam::ParamOnParseFunction BoolParam::set_on_parse_handler(BoolParam::ParamOnParseFunction on_parse_f) {
		BoolParam::ParamOnParseFunction rv = on_parse_f_;
		if (!on_parse_f)
			on_parse_f = BoolParam_ParamOnParseFunction;
		on_parse_f_ = on_parse_f;
		return rv;
	}
	template<>
	void BoolParam::clear_on_parse_handler() {
		on_parse_f_ = BoolParam_ParamOnParseFunction;
	}
	template<>
	BoolParam::ParamOnFormatFunction BoolParam::set_on_format_handler(BoolParam::ParamOnFormatFunction on_format_f) {
		BoolParam::ParamOnFormatFunction rv = on_format_f_;
		if (!on_format_f)
			on_format_f = BoolParam_ParamOnFormatFunction;
		on_format_f_ = on_format_f;
		return rv;
	}
	template<>
	void BoolParam::clear_on_format_handler() {
		on_format_f_ = BoolParam_ParamOnFormatFunction;
	}

#if 0
	template<>
	bool BoolParam::set_value(bool v, ParamSetBySourceType source_type, ParamPtr source) {
		bool val = (v != 0);
		return set_value(val, source_type, source);
	}

	// based on https://stackoverflow.com/questions/13698927/compare-double-to-zero-using-epsilon
#define inline_constexpr   inline
	static inline_constexpr double epsilon_plus()
	{
		const double a = 0.0;
		return std::nextafter(a, std::numeric_limits<double>::max());
	}
	static inline_constexpr double epsilon_minus()
	{
		const double a = 0.0;
		return std::nextafter(a, std::numeric_limits<double>::lowest());
	}
	static bool is_zero(const double b)
	{
		return epsilon_minus() <= b
			&& epsilon_plus() >= b;
	}

	template<>
	bool BoolParam::set_value(double v, ParamSetBySourceType source_type, ParamPtr source) {
		bool zero = is_zero(v);

		return set_value(!zero, source_type, source);
	}
#endif




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
#if defined(_MSC_VER)
		_set_errno(E_OK);
#else
		errno = E_OK;
#endif
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
			return nullptr;
		}
	}

	template<>
	DoubleParam::ValueTypedParam(const double value, THE_4_HANDLERS_PROTO)
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




	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// StringParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	void StringParam_ParamOnModifyFunction(StringParam &target, const std::string &old_value, std::string &new_value, const std::string &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter) {
		// nothing to do
		return;
	}

	void StringParam_ParamOnValidateFunction(StringParam &target, const std::string &old_value, std::string &new_value, const std::string &default_value, ParamSetBySourceType source_type) {
		// nothing to do
		return;
	}

	void StringParam_ParamOnParseFunction(StringParam &target, std::string &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type) {
		// we accept anything for a string parameter!
		new_value = source_value_str;
		pos = source_value_str.size();
	}

	std::string StringParam_ParamOnFormatFunction(const StringParam &source, const std::string &value, const std::string &default_value, Param::ValueFetchPurpose purpose) {
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
			return value;

			// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT:
			// Fetches the (formatted for print/display) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY:
			return default_value;

			// Return string representing the type of the parameter value, e.g. "integer".
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_INSPECT:
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY:
			return "string";

		default:
			DEBUG_ASSERT(0);
			return nullptr;
		}
	}

	template<>
	StringParam::StringTypedParam(const std::string &value, THE_4_HANDLERS_PROTO)
		: Param(name, comment, owner, init),
		on_modify_f_(on_modify_f ? on_modify_f : StringParam_ParamOnModifyFunction),
		on_validate_f_(on_validate_f ? on_validate_f : StringParam_ParamOnValidateFunction),
		on_parse_f_(on_parse_f ? on_parse_f : StringParam_ParamOnParseFunction),
		on_format_f_(on_format_f ? on_format_f : StringParam_ParamOnFormatFunction),
		value_(value),
		default_(value) {
		type_ = STRING_PARAM;
	}

	template<>
	StringParam::StringTypedParam(const std::string *value, THE_4_HANDLERS_PROTO)
		: StringTypedParam(value == nullptr ? "" : *value, name, comment, owner, init, on_modify_f, on_validate_f, on_parse_f, on_format_f)
	{}

	template<>
	StringParam::StringTypedParam(const char *value, THE_4_HANDLERS_PROTO)
		: StringTypedParam(std::string(value == nullptr ? "" : value), name, comment, owner, init, on_modify_f, on_validate_f, on_parse_f, on_format_f)
	{}

	template<>
	StringParam::operator const std::string &() const {
		return value();
	}

	template<>
	StringParam::operator const std::string *() const {
		return &value();
	}

	template<>
	const char* StringParam::c_str() const {
		return value().c_str();
	}

	template<>
	bool StringParam::empty() const noexcept {
		return value().empty();
	}

	// https://en.cppreference.com/w/cpp/feature_test#cpp_lib_string_contains
#if defined(__has_cpp_attribute) && __has_cpp_attribute(__cpp_lib_string_contains)  // C++23

	template<>
	bool StringParam::contains(char ch) const noexcept {
		return value().contains(ch);
	}

	template<>
	bool StringParam::contains(const char *s) const noexcept {
		return value().contains(s);
	}

	template<>
	bool StringParam::contains(const std::string &s) const noexcept {
		return value().contains(s);
	}

#else

	template<>
	bool StringParam::contains(char ch) const noexcept {
		auto v = value();
		auto f = v.find(ch);
		return f != std::string::npos;
	}

	template<>
	bool StringParam::contains(const char *s) const noexcept {
		auto v = value();
		auto f = v.find(s);
		return f != std::string::npos;
	}

	template<>
	bool StringParam::contains(const std::string &s) const noexcept {
		auto v = value();
		auto f = v.find(s);
		return f != std::string::npos;
	}

#endif

	template<>
	void StringParam::operator=(const std::string &value) {
		set_value(value, ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}

	template<>
	void StringParam::operator=(const std::string *value) {
		set_value((value == nullptr ? "" : *value), ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}

	template<>
	void StringParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		unsigned int pos = 0;
		std::string vs(v == nullptr ? "" : v);
		std::string vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, source_type); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			set_value(vv, source_type, source);
		}
	}

	template <>
	void StringParam::set_value(const std::string &val, ParamSetBySourceType source_type, ParamPtr source) {
		safe_inc(access_counts_.writing);
		// ^^^^^^^ --
		// Our 'writing' statistic counts write ATTEMPTS, in reailty.
		// Any real change is tracked by the 'changing' statistic (see further below)!

		std::string value(val);
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
	const std::string &StringParam::value() const noexcept {
		safe_inc(access_counts_.reading);
		return value_;
	}

	// Optionally the `source_vec` can be used to source the value to reset the parameter to.
	// When no source vector is specified, or when the source vector does not specify this
	// particular parameter, then our value is reset to the default value which was
	// specified earlier in our constructor.
	template<>
	void StringParam::ResetToDefault(const ParamsVectorSet *source_vec, ParamSetBySourceType source_type) {
		if (source_vec != nullptr) {
			StringParam *source = source_vec->find<StringParam>(name_str());
			if (source != nullptr) {
				set_value(source->value(), PARAM_VALUE_IS_RESET, source);
				return;
			}
		}
		set_value(default_, PARAM_VALUE_IS_RESET, nullptr);
	}

	template<>
	std::string StringParam::value_str(ValueFetchPurpose purpose) const {
		if (purpose == VALSTR_PURPOSE_DATA_4_USE)
			safe_inc(access_counts_.reading);
		return on_format_f_(*this, value_, default_, purpose);
	}

	template<>
	StringParam::ParamOnModifyFunction StringParam::set_on_modify_handler(StringParam::ParamOnModifyFunction on_modify_f) {
		StringParam::ParamOnModifyFunction rv = on_modify_f_;
		if (!on_modify_f)
			on_modify_f = StringParam_ParamOnModifyFunction;
		on_modify_f_ = on_modify_f;
		return rv;
	}
	template<>
	void StringParam::clear_on_modify_handler() {
		on_modify_f_ = StringParam_ParamOnModifyFunction;
	}
	template<>
	StringParam::ParamOnValidateFunction StringParam::set_on_validate_handler(StringParam::ParamOnValidateFunction on_validate_f) {
		StringParam::ParamOnValidateFunction rv = on_validate_f_;
		if (!on_validate_f)
			on_validate_f = StringParam_ParamOnValidateFunction;
		on_validate_f_ = on_validate_f;
		return rv;
	}
	template<>
	void StringParam::clear_on_validate_handler() {
		on_validate_f_ = StringParam_ParamOnValidateFunction;
	}
	template<>
	StringParam::ParamOnParseFunction StringParam::set_on_parse_handler(StringParam::ParamOnParseFunction on_parse_f) {
		StringParam::ParamOnParseFunction rv = on_parse_f_;
		if (!on_parse_f)
			on_parse_f = StringParam_ParamOnParseFunction;
		on_parse_f_ = on_parse_f;
		return rv;
	}
	template<>
	void StringParam::clear_on_parse_handler() {
		on_parse_f_ = StringParam_ParamOnParseFunction;
	}
	template<>
	StringParam::ParamOnFormatFunction StringParam::set_on_format_handler(StringParam::ParamOnFormatFunction on_format_f) {
		StringParam::ParamOnFormatFunction rv = on_format_f_;
		if (!on_format_f)
			on_format_f = StringParam_ParamOnFormatFunction;
		on_format_f_ = on_format_f;
		return rv;
	}
	template<>
	void StringParam::clear_on_format_handler() {
		on_format_f_ = StringParam_ParamOnFormatFunction;
	}

} // namespace tesseract
