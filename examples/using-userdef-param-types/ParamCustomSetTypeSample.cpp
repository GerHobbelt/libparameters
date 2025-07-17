
#include <parameters/parameters.h>

#include "internal_helpers.hpp"

// an example only...

using namespace parameters;

namespace {

	//
	// STAGE #1
	// 
	// DECLARE the custom parameter type (and it dummy fundamental referenced object + assistant object)
	//

	struct SampleObjType {
		int a;
		int b;

#if 0
		bool operator != (const SampleObjType & alt) {
			// dummy code
			return a != alt.a;
		}
#endif

		bool is_not_equal(const SampleObjType & alt) {
			// dummy code
			return a != alt.a;
		}
	};

	struct SampleObjAssistantType {
		void *x;
		void *y;
	};

	// plus the paramter type template instantiation itself: very handy using `using`:

	using SampleObjSetParam = ObjectVectorTypedParam<SampleObjType, SampleObjAssistantType>;

	// helper type

	using SampleObjVecType = std::vector<SampleObjType>;

	////////////////////////////////////////////////////////////////////////////////////////////////

	//
	// STAGE #2
	// 
	// DEFINE the essential callback functions (on_modify, on_validate, on_parse, on_format handlers)
	//

	void SampleObj_ParamOnModifyCFunction(SampleObjSetParam &target, const SampleObjVecType &old_value, SampleObjVecType &new_value, const SampleObjVecType &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter) {
		// dummy
	}

	void SampleObj_ParamOnValidateCFunction(SampleObjSetParam &target, const SampleObjVecType &old_value, SampleObjVecType &new_value, const SampleObjVecType &default_value, ParamSetBySourceType source_type) {
		// dummy
	}

	void SampleObj_ParamOnParseCFunction(SampleObjSetParam &target, SampleObjVecType &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type) {
		// dummy
	}

	std::string SampleObj_ParamOnFormatCFunction(const SampleObjSetParam &source, const SampleObjVecType &value, const SampleObjVecType& default_value, SampleObjSetParam::ValueFetchPurpose purpose) {
		return "dummy";
	}


	// plus a little hacky 'me iz empty/zero value' for use in the constructors below:
	static const SampleObjVecType nil{};

}  // namespace



//
// WARNING
//
// MSVC barfs a hairball when we try to define this class (= template instantion) in the anonymous namespace, so we temporarily turn on the original namespace for this:
//


namespace parameters {

	////////////////////////////////////////////////////////////////////////////////////////////////

	//
	// STAGE #3
	// 
	// DEFINE all methods + constructors as part of the template instantiation
	//

	template<>
	SampleObjSetParam::ObjectVectorTypedParam(const char *value, const SampleObjAssistantType &assist, const char *name, const char *comment, ParamsVector &owner, bool init, SampleObjSetParam::ParamOnModifyFunction on_modify_f, SampleObjSetParam::ParamOnValidateFunction on_validate_f, SampleObjSetParam::ParamOnParseFunction on_parse_f, SampleObjSetParam::ParamOnFormatFunction on_format_f)
		: Param(name, comment, owner, init)
		, on_modify_f_(on_modify_f)
		, on_validate_f_(on_validate_f)
		, on_parse_f_(on_parse_f)
		, on_format_f_(on_format_f)
		, assistant_(assist)
	{
		unsigned int pos = 0;
		std::string vs(value);
		SampleObjVecType vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, PARAM_VALUE_IS_DEFAULT); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			// do not use `set_value()` API as that one would be overhead and possibly count this write access
			value_ = vv;
			default_ = vv;
		}
	}

	template<>
	SampleObjSetParam::ObjectVectorTypedParam(const SampleObjVecType &value, const SampleObjAssistantType &assist, const char *name, const char *comment, ParamsVector &owner, bool init, SampleObjSetParam::ParamOnModifyFunction on_modify_f, SampleObjSetParam::ParamOnValidateFunction on_validate_f, SampleObjSetParam::ParamOnParseFunction on_parse_f, SampleObjSetParam::ParamOnFormatFunction on_format_f)
		: Param(name, comment, owner, init)
		, on_modify_f_(on_modify_f)
		, on_validate_f_(on_validate_f)
		, on_parse_f_(on_parse_f)
		, on_format_f_(on_format_f)
		, assistant_(assist)
		, value_(value)
		, default_(value)
	{}

	template<>
	SampleObjSetParam::~ObjectVectorTypedParam() = default;

	//operator T() const;
	template<>
	SampleObjSetParam::operator const SampleObjVecType&() const {
		return value();
	}
	template<>
	SampleObjSetParam::operator const SampleObjVecType *() const {
		return &value();
	}
	//void operator=(T value);
	template<>
	void SampleObjSetParam::operator=(const SampleObjVecType &value) {
		set_value(value, ParamUtils::get_current_application_default_param_source_type());
	}
	template<>
	void SampleObjSetParam::operator=(const SampleObjVecType *value) {
		const SampleObjVecType &v = (value != nullptr ? *value : nil);
		set_value(v, ParamUtils::get_current_application_default_param_source_type());
	}

	// Produce a reference to the parameter-internal assistant instance.
	// 
	// Used, for example, by the parse handler, to obtain info about delimiters, etc., necessary to successfully parse a string value into a T object.
	template<>
	SampleObjAssistantType &SampleObjSetParam::get_assistant() {
		return assistant_;
	}
	template<>
	const SampleObjAssistantType &SampleObjSetParam::get_assistant() const {
		return assistant_;
	}

	template<>
	SampleObjSetParam::operator const std::string &() {
		return "dummy";
	}
	template<>
	const char* SampleObjSetParam::c_str() const {
		return "dummy";
	}

	template<>
	bool SampleObjSetParam::empty() const noexcept {
		return false;
	}

	template<>
	void SampleObjSetParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		unsigned int pos = 0;
		std::string vs(v);
		SampleObjVecType vv;
		reset_fault();
		on_parse_f_(*this, vv, vs, pos, PARAM_VALUE_IS_DEFAULT); // minor(=recoverable) errors shall have signalled by calling fault()
		// when a signaled parse error occurred, we won't write the (faulty/undefined) value:
		if (!has_faulted()) {
			set_value(vv, source_type, source);
		}
	}
	template<>
	void SampleObjSetParam::set_value(const SampleObjVecType &val, ParamSetBySourceType source_type, ParamPtr source) {
		safe_inc(access_counts_.writing);
		// ^^^^^^^ --
		// Our 'writing' statistic counts write ATTEMPTS, in reailty.
		// Any real change is tracked by the 'changing' statistic (see further below)!

		SampleObjVecType value(val);
		reset_fault();
		// when we fail the validation horribly, the validator will throw an exception and thus abort the (write) action.
		// non-fatal errors may be signaled, in which case the write operation is aborted/skipped, or not signaled (a.k.a. 'silent')
		// in which case the write operation proceeds as if nothing untoward happened inside on_validate_f.
		on_validate_f_(*this, value_, value, default_, source_type);
		if (!has_faulted()) {
			// however, when we failed the validation only in the sense of the value being adjusted/restricted by the validator,
			// then we must set the value as set by the validator anyway, so nothing changes in our workflow here.

			set_ = (source_type > PARAM_VALUE_IS_RESET);
#ifdef __TODO__
			set_to_non_default_value_ = value.is_not_equal(default_);

			if (value.is_not_equal(value_)) {
#endif
				on_modify_f_(*this, value_, value, default_, source_type, source);
#ifdef __TODO__
				if (!has_faulted() && value.is_not_equal(value_)) {
#endif
					safe_inc(access_counts_.changing);
					value_ = value;
#ifdef __TODO__
				}
			}
#endif
		}
		// any signaled fault will be visible outside...
	}

	template<>
	const SampleObjVecType &SampleObjSetParam::value() const noexcept {
		safe_inc(access_counts_.reading);
		return value_;
	}

	// Optionally the `source_vec` can be used to source the value to reset the parameter to.
	// When no source vector is specified, or when the source vector does not specify this
	// particular parameter, then its value is reset to the default value which was
	// specified earlier in its constructor.
	template<>
	void SampleObjSetParam::ResetToDefault(const ParamsVectorSet *source_vec, ParamSetBySourceType source_type) {
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
	std::string SampleObjSetParam::value_str(ValueFetchPurpose purpose) const {
		if (purpose == VALSTR_PURPOSE_DATA_4_USE)
			safe_inc(access_counts_.reading);
		return on_format_f_(*this, value_, default_, purpose);
	}

	template<>
	SampleObjSetParam::ParamOnModifyFunction SampleObjSetParam::set_on_modify_handler(ParamOnModifyFunction on_modify_f) {
		auto rv = on_modify_f_;
		if (!on_modify_f)
			on_modify_f = SampleObj_ParamOnModifyCFunction;
		on_modify_f_ = on_modify_f;
		return rv;
	}
	template<>
	void SampleObjSetParam::clear_on_modify_handler() {
		on_modify_f_ = SampleObj_ParamOnModifyCFunction;
	}
	template<>
	SampleObjSetParam::ParamOnValidateFunction SampleObjSetParam::set_on_validate_handler(ParamOnValidateFunction on_validate_f) {
		auto rv = on_validate_f_;
		if (!on_validate_f)
			on_validate_f = SampleObj_ParamOnValidateCFunction;
		on_validate_f_ = on_validate_f;
		return rv;
	}
	template<>
	void SampleObjSetParam::clear_on_validate_handler() {
		on_validate_f_ = SampleObj_ParamOnValidateCFunction;
	}
	template<>
	SampleObjSetParam::ParamOnParseFunction SampleObjSetParam::set_on_parse_handler(ParamOnParseFunction on_parse_f) {
		auto rv = on_parse_f_;
		if (!on_parse_f)
			on_parse_f = SampleObj_ParamOnParseCFunction;
		on_parse_f_ = on_parse_f;
		return rv;
	}
	template<>
	void SampleObjSetParam::clear_on_parse_handler() {
		on_parse_f_ = SampleObj_ParamOnParseCFunction;
	}
	template<>
	SampleObjSetParam::ParamOnFormatFunction SampleObjSetParam::set_on_format_handler(ParamOnFormatFunction on_format_f) {
		auto rv = on_format_f_;
		if (!on_format_f)
			on_format_f = SampleObj_ParamOnFormatCFunction;
		on_format_f_ = on_format_f;
		return rv;
	}
	template<>
	void SampleObjSetParam::clear_on_format_handler() {
		on_format_f_ = SampleObj_ParamOnFormatCFunction;
	}




	////////////////////////////////////////////////////////////////////////////////////////////////


}  // namespace parameters


////////////////////////////////////////////////////////////////////////////////////////////////

//
// STAGE #4
// 
// back in our own anonymous namespace, *use* this new parameter type...
//

namespace {

	// declare a parameter instance (variable) and hook it up to the global set:
	SampleObjSetParam bugger1("", SampleObjAssistantType(), "bugger_it_millenium_hand_and_shrimp", "bla bla bla bla", GlobalParams());
	SampleObjSetParam bugger2("", SampleObjAssistantType(), "zero_man", "yak yak yak", GlobalParams());

	void using_it() {
		SampleObjVecType dddemo;
		dddemo[0].a = 42;

		bugger1 = dddemo;		// set parameter; *implicit* copy of the object.

#ifdef __TODO__
		dddemo[0].a = 666;
		bool is_neq = dddemo.is_not_equal(bugger1);
		LIBASSERT_ASSERT(is_neq == true);

		dddemo = bugger1;
		LIBASSERT_ASSERT(dddemo[0].a == 42);

#if 0
		SampleObjVecType &ref = bugger2;     // -- you cannot modify the parameterized object without the Param class being involved!
		ref.a = 13;
		dddemo.a = 13;
		is_neq = dddemo.is_not_equal(bugger2);
#else
		SampleObjVecType copy = bugger2;
		copy[0].a = 13;
		bugger2 = copy;
		dddemo[0].a = 13;
		is_neq = dddemo.is_not_equal(bugger2);
#endif
		LIBASSERT_ASSERT(is_neq == false);
#endif
	}


} // namespace 
