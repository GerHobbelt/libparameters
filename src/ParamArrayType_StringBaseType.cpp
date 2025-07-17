
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

#include <parameters/sourceref_defstart.h>

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// StringSetParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	void StringSetParam_ParamOnModifyFunction(StringSetParam &target, const std::vector<std::string> &old_value, std::vector<std::string> &new_value, const std::vector<std::string> &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter) {
		// nothing to do
		return;
	}

	void StringSetParam_ParamOnValidateFunction(StringSetParam &target, const std::vector<std::string> &old_value, std::vector<std::string> &new_value, const std::vector<std::string> &default_value, ParamSetBySourceType source_type) {
		// nothing to do
		return;
	}

	void StringSetParam_ParamOnParseFunction(StringSetParam &target, std::vector<std::string> &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type) {
		const BasicVectorParamParseAssistant &assistant = target.get_assistant();

		// create a modifiable copy of the `source_value_str`; we use a small-strings optimization approach similar to std::string internally.
		const char *svs = source_value_str.c_str();
		const int MAX_SMALLSIZE = 1022;
		char small_buf[MAX_SMALLSIZE + 2];
		const auto slen = strlen(svs);
		char *vs;
		if (slen <= MAX_SMALLSIZE) {
			vs = small_buf;
		} else {
			vs = new char[slen + 2];
		}
		// The value string will have a NUL sentinel at both ends while we process it.
		// This helps simplify and speed up the suffix checks below.
		*vs++ = 0;
		strcpy(vs, svs);
		DEBUG_ASSERT(vs[slen] == 0);

		// start parsing: `vs` points 1 NUL sentinel past the start of the allocated buffer space.

		// skip leading whitespace and any prefix:
		char *s = vs;
		while (isspace(*s))
			s++;
		bool has_display_prefix = false;
		const char *prefix = assistant.fmt_display_prefix.c_str();
		auto prefix_len = strlen(prefix);
		if (prefix_len && 0 == strncmp(s, prefix, prefix_len)) {
			s += prefix_len;
			while (isspace(*s))
				s++;
			has_display_prefix = true;
		} else {
			prefix = assistant.fmt_data_prefix.c_str();
			prefix_len = strlen(prefix);
			if (prefix_len && 0 == strncmp(s, prefix, prefix_len)) {
				s += prefix_len;
				while (isspace(*s))
					s++;
			}
		}
		// plug in a new before-start sentinel!
		// (We can do this safely as we allocated buffer space for this extra sentinel *and* started above by effectively writing a NUL sentinel at string position/index -1!)
		s[-1] = 0;

		// now perform the mirror action by checking and skipping any trailing whitespace and suffix!
		// (When the source string doesn't contain anything else, this code still works great for it will hit the *start sentinnel*!)
		char *e = s + strlen(s) - 1;
		while (isspace(*e))
			e--;
		// plug in a new sentinel!
		*++e = 0;

		const char *suffix = has_display_prefix ? assistant.fmt_display_postfix.c_str() : assistant.fmt_data_postfix.c_str();
		auto suffix_len = strlen(suffix);
		e -= suffix_len;
		if (suffix_len && e >= s && 0 == strcmp(e, suffix)) {
			e--;
			while (isspace(*e))
				e--;
			e++;

			// plug in a new sentinel!
			e[0] = 0;
		} else {
			e += suffix_len;
		}

		// now `s` points at the first value in the input string and `e` points at the end sentinel, just beyond the last value in the input string.
		DEBUG_ASSERT(s == e ? *s == 0 : *s != 0);
		new_value.clear();
		const char *delimiters = assistant.parse_separators.c_str();
		while (s < e) {
			// leading whitespace removel; only relevant for the 2nd element and beyond as we already stripped leading whitespace for the first element in the prefix-skipping code above.
			while (isspace(*s))
				s++;
			auto n = strcspn(s, delimiters);
			char *ele = s + n;
			// plug in a new end-of-element sentinel!
			*ele++ = 0;
			if (n) {
				// there's actual content here, so we can expect trailing whitespace to follow it: trim it.
				char *we = ele - 2;
				while (isspace(*we))
					we--;
				we++;
				// plug in a new end-of-element sentinel!
				*we = 0;
			}
			// we DO NOT accept empty (string) element values!
			if (*s) {
				new_value.push_back(s);
			}
			s = ele;
		}
		// All done, no boogers.
		pos = slen;
	}

	static inline std::string fmt_stringset_vector(const std::vector<std::string> &value, const char *prefix, const char *suffix, const char *separator) {
		std::string rv;
		rv = prefix;
		for (const std::string &elem : value) {
			rv += elem;
			rv += separator;
		}
		if (value.size()) {
			// we pushed one separator too many: roll back
			for (size_t i = 0, en = strlen(separator); i < en; i++) {
				(void)rv.pop_back();
			}
		}
		rv += suffix;
		return rv;
	}

	std::string StringSetParam_ParamOnFormatFunction(const StringSetParam &source, const std::vector<std::string> &value, const std::vector<std::string> &default_value, Param::ValueFetchPurpose purpose) {
		const BasicVectorParamParseAssistant &assistant = source.get_assistant();
		switch (purpose) {
			// Fetches the (raw, parseble for re-use via set_value()) value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_RAW_DATA_4_INSPECT:
			// Fetches the (raw, parseble for re-use via set_value() or storing to serialized text data format files) value of the param as a string.
			//
			// NOTE: The part where the documentation says this variant MUST update the parameter usage statistics is
			// handled by the Param class code itself; no need for this callback to handle that part of the deal.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DATA_4_USE:
			return fmt_stringset_vector(value, assistant.fmt_data_prefix.c_str(), assistant.fmt_data_postfix.c_str(), assistant.fmt_data_separator.c_str());

			// Fetches the (formatted for print/display) value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DATA_FORMATTED_4_DISPLAY:
			return fmt_stringset_vector(value, assistant.fmt_display_prefix.c_str(), assistant.fmt_display_postfix.c_str(), assistant.fmt_display_separator.c_str());

			// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT:
			return fmt_stringset_vector(default_value, assistant.fmt_data_prefix.c_str(), assistant.fmt_data_postfix.c_str(), assistant.fmt_data_separator.c_str());

			// Fetches the (formatted for print/display) default value of the param as a string.
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY:
			return fmt_stringset_vector(default_value, assistant.fmt_display_prefix.c_str(), assistant.fmt_display_postfix.c_str(), assistant.fmt_display_separator.c_str());

			// Return string representing the type of the parameter value, e.g. "integer".
		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_INSPECT:
			return "StringArray";

		case Param::ValueFetchPurpose::VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY:
			return "set of strings";

		default:
			DEBUG_ASSERT(0);
			return {};
		}
	}


	template<>
	StringSetParam::BasicVectorTypedParam(const std::vector<std::string> &value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_IMPL)
		: Param(name, comment, owner, init),
		on_modify_f_(on_modify_f ? on_modify_f : StringSetParam_ParamOnModifyFunction),
		on_validate_f_(on_validate_f ? on_validate_f : StringSetParam_ParamOnValidateFunction),
		on_parse_f_(on_parse_f ? on_parse_f : StringSetParam_ParamOnParseFunction),
		on_format_f_(on_format_f ? on_format_f : StringSetParam_ParamOnFormatFunction),
		value_(value),
		default_(value),
		assistant_(assistant) {
		type_ = STRING_SET_PARAM;
	}

	template<>
	StringSetParam::BasicVectorTypedParam(const char *value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_IMPL)
		: BasicVectorTypedParam(std::vector<std::string>(), assistant, name, comment, owner, init, on_modify_f, on_validate_f, on_parse_f, on_format_f)
	{
		unsigned int pos = 0;
		std::string vs(value == nullptr ? "" : value);
		std::vector<std::string> vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, PARAM_VALUE_IS_DEFAULT); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			//set_value(vv, PARAM_VALUE_IS_DEFAULT, nullptr);
			value_ = vv;
		}
	}

	template<>
	StringSetParam::operator const std::vector<std::string> &() const {
		return value();
	}

	template<>
	StringSetParam::operator const std::vector<std::string> *() const {
		return &value();
	}

	template<>
	const char *StringSetParam::c_str() const {
		return value_str(VALSTR_PURPOSE_DATA_4_USE).c_str();
	}

	template<>
	bool StringSetParam::empty() const noexcept {
		return value().empty();
	}

	template<>
	void StringSetParam::operator=(const std::vector<std::string> &value) {
		set_value(value, ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}

	template<>
	void StringSetParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		unsigned int pos = 0;
		std::string vs(v == nullptr ? "" : v);
		std::vector<std::string> vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, source_type); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			set_value(vv, source_type, source);
		}
	}

	template <>
	void StringSetParam::set_value(const std::vector<std::string> &val, ParamSetBySourceType source_type, ParamPtr source) {
		safe_inc(access_counts_.writing);
		// ^^^^^^^ --
		// Our 'writing' statistic counts write ATTEMPTS, in reailty.
		// Any real change is tracked by the 'changing' statistic (see further below)!

		std::vector<std::string> value(val);
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
	const std::vector<std::string> &StringSetParam::value() const noexcept {
		safe_inc(access_counts_.reading);
		return value_;
	}

	// Optionally the `source_vec` can be used to source the value to reset the parameter to.
	// When no source vector is specified, or when the source vector does not specify this
	// particular parameter, then our value is reset to the default value which was
	// specified earlier in our constructor.
	template <>
	void StringSetParam::ResetToDefault(const ParamsVectorSet *source_vec, ParamSetBySourceType source_type) {
		if (source_vec != nullptr) {
			StringSetParam *source = source_vec->find<StringSetParam>(name_str());
			if (source != nullptr) {
				set_value(source->value(), PARAM_VALUE_IS_RESET, source);
				return;
			}
		}
		set_value(default_, PARAM_VALUE_IS_RESET, nullptr);
	}

	template<>
	std::string StringSetParam::value_str(ValueFetchPurpose purpose) const {
		if (purpose == VALSTR_PURPOSE_DATA_4_USE)
			safe_inc(access_counts_.reading);
		return on_format_f_(*this, value_, default_, purpose);
	}

	template<>
	StringSetParam::ParamOnModifyFunction StringSetParam::set_on_modify_handler(StringSetParam::ParamOnModifyFunction on_modify_f) {
		StringSetParam::ParamOnModifyFunction rv = on_modify_f_;
		if (!on_modify_f)
			on_modify_f = StringSetParam_ParamOnModifyFunction;
		on_modify_f_ = on_modify_f;
		return rv;
	}
	template<>
	void StringSetParam::clear_on_modify_handler() {
		on_modify_f_ = StringSetParam_ParamOnModifyFunction;
	}
	template<>
	StringSetParam::ParamOnValidateFunction StringSetParam::set_on_validate_handler(StringSetParam::ParamOnValidateFunction on_validate_f) {
		StringSetParam::ParamOnValidateFunction rv = on_validate_f_;
		if (!on_validate_f)
			on_validate_f = StringSetParam_ParamOnValidateFunction;
		on_validate_f_ = on_validate_f;
		return rv;
	}
	template<>
	void StringSetParam::clear_on_validate_handler() {
		on_validate_f_ = StringSetParam_ParamOnValidateFunction;
	}
	template<>
	StringSetParam::ParamOnParseFunction StringSetParam::set_on_parse_handler(StringSetParam::ParamOnParseFunction on_parse_f) {
		StringSetParam::ParamOnParseFunction rv = on_parse_f_;
		if (!on_parse_f)
			on_parse_f = StringSetParam_ParamOnParseFunction;
		on_parse_f_ = on_parse_f;
		return rv;
	}
	template<>
	void StringSetParam::clear_on_parse_handler() {
		on_parse_f_ = StringSetParam_ParamOnParseFunction;
	}
	template<>
	StringSetParam::ParamOnFormatFunction StringSetParam::set_on_format_handler(StringSetParam::ParamOnFormatFunction on_format_f) {
		StringSetParam::ParamOnFormatFunction rv = on_format_f_;
		if (!on_format_f)
			on_format_f = StringSetParam_ParamOnFormatFunction;
		on_format_f_ = on_format_f;
		return rv;
	}
	template<>
	void StringSetParam::clear_on_format_handler() {
		on_format_f_ = StringSetParam_ParamOnFormatFunction;
	}

#if 0
	std::string StringSetParam::formatted_value_str() const {
		std::string rv = "\u00AB";
		rv += value_;
		rv += "\u00BB";
		return std::move(rv);
	}
#endif





#include <parameters/sourceref_defend.h>

} // namespace tesseract
