
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

#include <parameters/sourceref_defstart.h>

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
			return {};
		}
	}

	template<>
	StringParam::StringTypedParam(const std::string &value, THE_4_HANDLERS_PROTO_4_IMPL)
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
	StringParam::StringTypedParam(const std::string *value, THE_4_HANDLERS_PROTO_4_IMPL)
		: StringTypedParam(value == nullptr ? "" : *value, name, comment, owner, init, on_modify_f, on_validate_f, on_parse_f, on_format_f)
	{}

	template<>
	StringParam::StringTypedParam(const char *value, THE_4_HANDLERS_PROTO_4_IMPL)
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
	//
	// Augmented for other compilers than just GCC:
#if defined(__has_cpp_attribute) && defined(__GNUG__)
# define HAS_COMPILER_ATTRIBUTE(name)  __has_cpp_attribute(name)
#else
# define HAS_COMPILER_ATTRIBUTE(name)  (name > 0)
#endif

#if defined(__has_cpp_attribute) && defined(__cpp_lib_string_contains) && HAS_COMPILER_ATTRIBUTE(__cpp_lib_string_contains)  // C++23

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

#include <parameters/sourceref_defend.h>

} // namespace tesseract
