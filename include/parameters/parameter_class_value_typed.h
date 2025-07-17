/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_CLASSES_VALUE_TYPED_H_
#define _LIB_PARAMS_CLASSES_VALUE_TYPED_H_

#include <parameters/parameter_class_fundamentals.h>
#include <parameters/parameter_class_base.h>
#include <parameters/fmt-support.h>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>


namespace parameters {

#include <parameters/sourceref_defstart.h>

	// --------------------------------------------------------------------------------------------------

/*
 * NOTE: a previous version of these typed parameter classes used C++ templates, but I find that templates cannot do one thing:
 * make sure that the copy and move constructors + operator= methods for the *final class* are matched, unless such a class is
 * only a `using` statement of a template instantiation.
 *
 * Hence we succumb to using preprocessor macros below instead, until someone better versed in C++ than me comes along and keeps things readable; I didn't succeed
 * for the RefTypeParam-based StringSetParam and IntSetParam classes, so those are produced with some help from the preprocessor
 * instead.
 */

	// Using this one as the base for fundamental types:
	template <class T, class Assistant>
	class ValueTypedParam: public Param {
		using RTP = ValueTypedParam<T, Assistant>;

	public:
		using Param::Param;
		using Param::operator=;

		// Return when modify/write action may proceed; throw an exception on (non-recovered) error. `new_value` MAY have been adjusted by this modify handler. The modify handler is not supposed to modify any read/write/modify access accounting data. Minor infractions (which resulted in some form of recovery) may be signaled by flagging the parameter state via its fault() API method.
		typedef void ParamOnModifyCFunction(RTP &target, const T old_value, T &new_value, const T default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return when validation action passed and modify/write may proceed; throw an exception on (non-recovered) error. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data. Minor infractions (which resulted in some form of recovery) may be signaled by flagging the parameter state via its fault() API method.
		typedef void ParamOnValidateCFunction(RTP &target, const T old_value, T &new_value, const T default_value, ParamSetBySourceType source_type);

		// Return when the parse action (parsing `source_value_str` starting at offset `pos`) completed successfully or required only minor recovery; throw an exception on (non-recovered) error.
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		// Minor infractions (which resulted in some form of recovery) may be signaled by flagging the parameter state via its fault() API method.
		typedef void ParamOnParseCFunction(RTP& target, T& new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		// This formatting action is supposed to always succeed or fail fatally (e.g. out of heap memory) by throwing an exception.
		// The formatter implementation is not supposed to signal any errors via the fault() API method.
		typedef std::string ParamOnFormatCFunction(const RTP &source, const T value, const T default_value, ValueFetchPurpose purpose);

		using ParamOnModifyFunction = std::function<ParamOnModifyCFunction>;
		using ParamOnValidateFunction = std::function<ParamOnValidateCFunction>;
		using ParamOnParseFunction = std::function<ParamOnParseCFunction>;
		using ParamOnFormatFunction = std::function<ParamOnFormatCFunction>;

	public:
		ValueTypedParam(const char *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		ValueTypedParam(const T value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		explicit ValueTypedParam(const T *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		virtual ~ValueTypedParam() = default;

		operator T() const;
		operator const T&() const;
		//operator const T *() const;
		void operator=(const T value);
		void operator=(const T &value);
		//void operator=(const T *value);

		// Produce a reference to the parameter-internal assistant instance.
		// 
		// Used, for example, by the parse handler, to obtain info about delimiters, etc., necessary to successfully parse a string value into a T object.
		Assistant &get_assistant();
		const Assistant &get_assistant() const;

		operator const std::string &();
		const char* c_str() const;

		bool empty() const noexcept;

		virtual void set_value(const char *v, SOURCE_REF) override;
		void set_value(const T v, SOURCE_REF);

		// the Param::set_value methods will not be considered by the compiler here, resulting in at least 1 compile error in params.cpp,
		// due to this nasty little blurb:
		//
		// > Member lookup rules are defined in Section 10.2/2
		// >
		// > The following steps define the result of name lookup in a class scope, C.
		// > First, every declaration for the name in the class and in each of its base class sub-objects is considered. A member name f
		// > in one sub-object B hides a member name f in a sub-object A if A is a base class sub-object of B. Any declarations that are
		// > so hidden are eliminated from consideration.          <-- !!!
		// > Each of these declarations that was introduced by a using-declaration is considered to be from each sub-object of C that is
		// > of the type containing the declara-tion designated by the using-declaration. If the resulting set of declarations are not
		// > all from sub-objects of the same type, or the set has a nonstatic member and includes members from distinct sub-objects,
		// > there is an ambiguity and the program is ill-formed. Otherwise that set is the result of the lookup.
		//
		// Found here: https://stackoverflow.com/questions/5368862/why-do-multiple-inherited-functions-with-same-name-but-different-signatures-not
		// which seems to be off-topic due to the mutiple-inheritance issues discussed there, but the phrasing of that little C++ standards blurb
		// is such that it applies to our situation as well, where we only replace/override a *subset* of the available set_value() methods from
		// the Params class. Half a year later and I stumble across that little paragraph; would never have thought to apply a `using` statement
		// here, but it works! !@#$%^&* C++!
		//
		// Incidentally, the fruity thing about it all is that it only errors out for StringParam in params.cpp, while a sane individual would've
		// reckoned it'd bother all four of them: IntParam, FloatParam, etc.
		using Param::set_value;

		T value() const noexcept;

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = nullptr, SOURCE_TYPE) override;
		using Param::ResetToDefault;

		virtual std::string value_str(ValueFetchPurpose purpose) const override;

		ValueTypedParam(const RTP &o) = delete;
		ValueTypedParam(RTP &&o) = delete;
		ValueTypedParam(const RTP &&o) = delete;

		RTP &operator=(const RTP &other) = delete;
		RTP &operator=(RTP &&other) = delete;
		RTP &operator=(const RTP &&other) = delete;

		ParamOnModifyFunction set_on_modify_handler(ParamOnModifyFunction on_modify_f);
		void clear_on_modify_handler();
		ParamOnValidateFunction set_on_validate_handler(ParamOnValidateFunction on_validate_f);
		void clear_on_validate_handler();
		ParamOnParseFunction set_on_parse_handler(ParamOnParseFunction on_parse_f);
		void clear_on_parse_handler();
		ParamOnFormatFunction set_on_format_handler(ParamOnFormatFunction on_format_f);
		void clear_on_format_handler();

	protected:
		ParamOnModifyFunction on_modify_f_;
		ParamOnValidateFunction on_validate_f_;
		ParamOnParseFunction on_parse_f_;
		ParamOnFormatFunction on_format_f_;

	protected:
		T value_;
		T default_;
		Assistant assistant_;
	};

	// --------------------------------------------------------------------------------------------------

	// remove the macros to help set up the member prototypes

#include <parameters/sourceref_defend.h>

} // namespace 

#endif
