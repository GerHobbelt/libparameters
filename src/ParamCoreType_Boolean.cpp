
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

#include <parameters/sourceref_defstart.h>

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
		clear_errno();
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

	std::string BoolParam_ParamOnFormatFunction(const BoolParam &source, const bool value, const bool default_value, ValueFetchPurpose purpose) {
		switch (purpose) {
			// Fetches the (raw, parseble for re-use via set_value()) value of the param as a string.
		case ValueFetchPurpose::VALSTR_PURPOSE_RAW_DATA_4_INSPECT:
			// Fetches the (formatted for print/display) value of the param as a string.
		case ValueFetchPurpose::VALSTR_PURPOSE_DATA_FORMATTED_4_DISPLAY:
			// Fetches the (raw, parseble for re-use via set_value() or storing to serialized text data format files) value of the param as a string.
			//
			// NOTE: The part where the documentation says this variant MUST update the parameter usage statistics is
			// handled by the Param class code itself; no need for this callback to handle that part of the deal.
		case ValueFetchPurpose::VALSTR_PURPOSE_DATA_4_USE:
			return value ? "true" : "false";

			// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string.
		case ValueFetchPurpose::VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT:
			// Fetches the (formatted for print/display) default value of the param as a string.
		case ValueFetchPurpose::VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY:
			return default_value ? "true" : "false";

			// Return string representing the type of the parameter value, e.g. "integer".
		case ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_INSPECT:
			return "bool";

		case ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY:
			return "boolean";

		default:
			DEBUG_ASSERT(0);
			return {};
		}
	}

	template<>
	BoolParam::ValueTypedParam(const bool value, THE_4_HANDLERS_PROTO_4_IMPL)
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



#include <parameters/sourceref_defend.h>

} // namespace tesseract
