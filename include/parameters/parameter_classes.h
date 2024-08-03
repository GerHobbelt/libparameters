/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_CLASSES_H_
#define _LIB_PARAMS_CLASSES_H_

#include <parameters/fmt-support.h>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>


namespace parameters {

	class Param;
	class ParamsVector;
	class ParamsVectorSet;

	// The value types supported by the Param class hierarchy. These identifiers can be bitwise-OR-ed
	// to form a primitive selection/filter mask, as used in the `find()` functions and elsewhere.
	enum ParamType {
		UNKNOWN_PARAM = 0,

		INT_PARAM =         0x0001,            // 32-bit signed integer
		BOOL_PARAM =        0x0002,
		DOUBLE_PARAM =      0x0004,
		STRING_PARAM =      0x0008,
		STRING_SET_PARAM =  0x0010,
		INT_SET_PARAM =     0x0020,
		BOOL_SET_PARAM =    0x0040,
		DOUBLE_SET_PARAM =  0x0080,
		CUSTOM_PARAM =      0x0100, // a yet-unspecified type: provided as an advanced-use generic parameter value storage container for when the other, basic, value types do not suffice in userland code. The tesseract core does not employ this value type anywhere: we do have compound paramater values, mostly sets of file paths, but those are encoded as *string* value in their parameter value.
		CUSTOM_SET_PARAM =  0x0200, // a yet-unspecified vector type.

		ANY_TYPE_PARAM =    0x03FF, // catch-all identifier for the selection/filter functions: there this is used to match *any and all* parameter value types encountered.
	};
	DECL_FMT_FORMAT_PARAMENUMTYPE(ParamType);

	static inline auto format_as(ParamType t) {
		return fmt::underlying(t);
	}

	// Identifiers used to indicate the *origin* of the current parameter value. Used for reporting/diagnostic purposes. Do not treat these
	// as gospel; these are often assigned under limited/reduced information conditions, so they merely serve as report *hints*.
	enum ParamSetBySourceType {
		PARAM_VALUE_IS_DEFAULT = 0,

		PARAM_VALUE_IS_RESET,
		PARAM_VALUE_IS_SET_BY_PRESET,         // 'indirect' write: a tesseract 'preset' parameter set was invoked and that one set this one as part of the action.
		PARAM_VALUE_IS_SET_BY_CONFIGFILE,     // 'explicit' write by loading and processing a config file.
		PARAM_VALUE_IS_SET_BY_ASSIGN,			    // 'indirect' write: value is copied over from elsewhere via operator=.
		PARAM_VALUE_IS_SET_BY_PARAM,          // 'indirect' write: other Param's OnChange code set the param value, whatever it is now.
		PARAM_VALUE_IS_SET_BY_APPLICATION,    // 'explicit' write: user / application code set the param value, whatever it is now.
		PARAM_VALUE_IS_SET_BY_CORE_RUN,       // 'explicit' write by the running application core: before proceding with the next step the run-time adjusts this one, e.g. (incrementing) page number while processing a multi-page OCR run.
		PARAM_VALUE_IS_SET_BY_SNAPSHOT_REWIND, // 'explicit' write: parameter snapshot was rewound to a given snapshot/value.
	};
	DECL_FMT_FORMAT_PARAMENUMTYPE(ParamSetBySourceType);

	static inline auto format_as(ParamSetBySourceType t) {
		return fmt::underlying(t);
	}

	// --------------------------------------------------------------------------------------------------

	// Readability helper types: reference and pointer to `Param` base class.
	typedef Param & ParamRef;
	typedef Param * ParamPtr;

#include <parameters/sourceref_defstart.h>

	typedef void *ParamVoidPtrDataType;

	struct ParamArbitraryOtherType {
		void *data_ptr;
		size_t data_size;
		size_t extra_size;
		void *extra_ptr;
	};

	typedef std::vector<std::string> ParamStringSetType;
	typedef std::vector<int32_t> ParamIntSetType;
	typedef std::vector<bool> ParamBoolSetType;
	typedef std::vector<double> ParamDoubleSetType;

	// --- section setting up various C++ template constraint helpers ---
	//
	// These assist C++ authors to produce viable template instances. 

#if defined(__cpp_concepts)

#if 0

	template<class T>struct tag_t {};
	template<class T>constexpr tag_t<T> tag{};
	namespace detect_string {
		template<class T, class...Ts>
		constexpr bool is_stringlike(tag_t<T>, Ts&&...) { return false; }
		template<class T, class A>
		constexpr bool is_stringlike(tag_t<std::basic_string<T, A>>) { return true; }
		template<class T>
		constexpr bool detect = is_stringlike(tag<T>); // enable ADL extension
	}
	namespace detect_character {
		template<class T, class...Ts>
		constexpr bool is_charlike(tag_t<T>, Ts&&...) { return false; }
		constexpr bool is_charlike(tag_t<char>) { return true; }
		constexpr bool is_charlike(tag_t<wchar_t>) { return true; }
		// ETC
		template<class T>
		constexpr bool detect = is_charlike(tag<T>); // enable ADL extension
	}

#endif

	// as per https://stackoverflow.com/questions/874298/how-do-you-constrain-a-template-to-only-accept-certain-types
	template<typename T>
	concept ParamAcceptableValueType = std::is_integral<T>::value
		//|| std::is_base_of<bool, T>::value 
		|| std::is_floating_point<T>::value
		// || std::is_base_of<char*, T>::value  // fails as per https://stackoverflow.com/questions/23986784/why-is-base-of-fails-when-both-are-just-plain-char-type
		// || std::is_same<char*, T>::value
		// || std::is_same<const char*, T>::value
		//|| std::is_nothrow_convertible<char*, T>::value
		//|| std::is_nothrow_convertible<const char*, T>::value
		|| std::is_nothrow_convertible<bool, T>::value
		|| std::is_nothrow_convertible<double, T>::value
		|| std::is_nothrow_convertible<int32_t, T>::value
		//|| std::is_base_of<std::string, T>::value
		;

	template <typename T>
	concept ParamAcceptableOtherType = !std::is_integral<T>::value
		&& !std::is_base_of<bool, T>::value
		&& !std::is_floating_point<T>::value
		&& !std::is_base_of<char*, T>::value  // fails as per https://stackoverflow.com/questions/23986784/why-is-base-of-fails-when-both-are-just-plain-char-type
		&& !std::is_same<char*, T>::value
		&& !std::is_same<const char*, T>::value
		//|| std::is_nothrow_convertible<char*, T>::value
		//|| std::is_nothrow_convertible<const char*, T>::value
		//|| std::is_nothrow_convertible<bool, T>::value || std::is_nothrow_convertible<double, T>::value || std::is_nothrow_convertible<int32_t, T>::value
		&& !std::is_base_of<std::string, T>::value
		;

	static_assert(ParamAcceptableValueType<int>);
	static_assert(ParamAcceptableValueType<double>);
	static_assert(ParamAcceptableValueType<bool>);
	static_assert(!ParamAcceptableValueType<const char *>);
	static_assert(!ParamAcceptableValueType<char*>);
	static_assert(!ParamAcceptableValueType<std::string>);
	static_assert(!ParamAcceptableValueType<std::string&>);
	static_assert(ParamAcceptableOtherType<ParamVoidPtrDataType>);
	static_assert(ParamAcceptableOtherType<ParamArbitraryOtherType>);
	static_assert(ParamAcceptableOtherType<ParamStringSetType>);
	static_assert(ParamAcceptableOtherType<ParamIntSetType>);
	static_assert(ParamAcceptableOtherType<ParamBoolSetType>);
	static_assert(ParamAcceptableOtherType<ParamDoubleSetType>);

	//template<typename T>
	//concept ParamDerivativeType = std::is_base_of<Param, T>::value;

#define ParamDerivativeType   typename

#else

#define ParamAcceptableValueType   class

#define ParamDerivativeType   class

#endif

// --- END of section setting up various C++ template constraint helpers ---


// --------------------------------------------------------------------------------------------------

// A set (vector) of parameters. While this is named *vector* the internal organization
// is hash table based to provide fast random-access add / remove / find functionality.
	class ParamsVector;

	// --------------------------------------------------------------------------------------------------

	// an (ad-hoc?) collection of ParamsVector instances.
	class ParamsVectorSet;

	// --------------------------------------------------------------------------------------------------

	// Definition of various parameter types.
	class Param {
	protected:
		Param(const char *name, const char *comment, ParamsVector &owner, bool init = false);

	public:
		virtual ~Param();

		const char *name_str() const noexcept;
		const char *info_str() const noexcept;
		bool is_init() const noexcept;
		bool is_debug() const noexcept;
		bool is_set() const noexcept;
		bool is_set_to_non_default_value() const noexcept;
		bool is_locked() const noexcept;
		bool has_faulted() const noexcept;

		void lock(bool locking = true);

		bool can_update(ParamSetBySourceType source_type) const noexcept;

		// Signal a (recoverable) fault; used, together with has_faulted() and reset_fault(), by the parameter classes' internals when,
		// f.e., a string value doesn't parse or fails to pass a parameter's validation checks.
		//
		// This signal remains signaled until the next call to reset_fault().
		// DO NOTE that any subsequent parameter value *write* operations for this parameter will internally *reset* the fault state,
		// thus using a clean slate against which the next parse+write (validation+modify checks) will be checked for (new) faults.
		//
		// DO NOTE that each call to `fault()` will bump the error signal count (statistics) for this parameter, hence any 
		// application-level reporting of parameter usage statistics will not miss to report these faults from having occurred.
		void fault() noexcept;

		// Reset the fault state of this parameter, so any subsequent write operation will not be aborted/skipped any more.
		// 
		// Is used to turn the fault signaled state OFF and parse+write a new incoming value.
		void reset_fault() noexcept;

		ParamSetBySourceType set_mode() const noexcept;
		Param *is_set_by() const noexcept;

		ParamsVector &owner() const noexcept;

		// We track Param/Variable setup/changes/usage through this administrative struct.
		// It helps us to diagnose and report which tesseract Params (Variables) are actually
		// USED in which program section and in the program as a whole, while we can also 
		// diagnose which variables have been set up during this session, and by *whom*, as
		// some Params will have been modified due to others having been set, e.g. `debug_all`.
		typedef struct access_counts {
			// the current section's counts
			uint16_t reading;
			uint16_t writing;   // counting the number of *write* actions, answering the question "did we assign a value to this one during this run?"
			uint16_t changing;  // counting the number of times a *write* action resulted in an actual *value change*, answering the question "did we use a non-default value for this one during this run?"
			uint16_t faulting;  // counting the number of times a *parse* action produced a *fault* instead of a legal value to be written into the parameter.
		} access_counts_t;

		const access_counts_t &access_counts() const noexcept;

		// Reset the access count statistics in preparation for the next run.
		// As a side effect the current run's access count statistics will be added to the history
		// set, available via the `prev_sum_*` access_counts_t members.
		void reset_access_counts() noexcept;

		enum ValueFetchPurpose {
			// Fetches the (raw, parseble for re-use via set_value()) value of the param as a string and does not add
			// this access to the read counter tally. This is useful, f.e., when checking 
			// parameters' values before deciding to modify them via a CLI, UI interface or config file (re)load.
			//
			// We do not count this read access as this method is for *validation/comparison* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_RAW_DATA_4_INSPECT,

			// Fetches the (formatted for print/display) value of the param as a string and does not add
			// this access to the read counter tally. This is useful, f.e., when printing 'init'
			// (only-settable-before-first-use) parameters to config file or log file, independent
			// from the actual work process.
			//
			// We do not count this read access as this method is for *display* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_DATA_FORMATTED_4_DISPLAY,

			// Fetches the (raw, parseble for re-use via set_value() for storing to serialized text data format files) value of the param as a string and DOES add
			// this access to the read counter tally. This is useful, f.e., when printing 'init'
			// (only-settable-before-first-use) parameters to config file, independent
			// from the actual work process.
			//
			// We do not count this read access as this method is for *validation/comparison* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_DATA_4_USE,

			// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string and does not add
			// this access to the read counter tally. This is useful, f.e., when checking
			// parameters' values before deciding to modify them via a CLI, UI interface or config file (re)load.
			//
			// We do not count this read access as this method is for *validation/comparison* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT,

			// Fetches the (formatted for print/display) default value of the param as a string and does not add
			// this access to the read counter tally. This is useful, f.e., when printing 'init'
			// (only-settable-before-first-use) parameters to config file or log file, independent
			// from the actual work process.
			//
			// We do not count this read access as this method is for *display* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY,

			// Return string representing the type of the parameter value, e.g. "integer"
			//
			// We do not count this read access as this method is for *print/write-to-file* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_TYPE_INFO_4_INSPECT,

			// Return string representing the type of the parameter value, e.g. "integer"
			//
			// We do not count this read access as this method is for *display* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY,
		};

		// Fetches the (possibly formatted) value of the param as a string; see the ValueFetchPurpose
		// enum documentation for detailed info which purposes are counted in the access statistics
		// and which aren't.
		virtual std::string value_str(ValueFetchPurpose purpose) const = 0;

		// Fetches the (formatted for print/display) value of the param as a string and does not add 
		// this access to the read counter tally. This is useful, f.e., when printing 'init' 
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		//
		// We do not count this read access as this method is for *display* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string formatted_value_str() const;

		// Fetches the (raw, parseble for re-use via set_value()) value of the param as a string and does not add 
		// this access to the read counter tally. This is useful, f.e., when printing 'init' 
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process. 
		//
		// We do not count this read access as this method is for *validation/comparison* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string raw_value_str() const;

		// Fetches the (formatted for print/display) default value of the param as a string and does not add
		// this access to the read counter tally. This is useful, f.e., when printing 'init'
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		//
		// We do not count this read access as this method is for *display* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string formatted_default_value_str() const;

		// Fetches the (raw, parseble for re-use via set_value()) default value of the param as a string and does not add
		// this access to the read counter tally. This is useful, f.e., when printing 'init'
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		//
		// We do not count this read access as this method is for *validation/comparison* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string raw_default_value_str() const;

		// Return string representing the type of the parameter value, e.g. "integer"
		//
		// We do not count this read access as this method is for *display* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string value_type_str() const;

		// Return string representing the type of the parameter value, e.g. "integer"
		//
		// We do not count this read access as this method is for *print/save-to-file* purposes only and we do not
		// wish to tally those together with the actual work code accessing this parameter through
		// the other functions: set_value() and assignment operators.
		std::string raw_value_type_str() const;

		virtual void set_value(const char *v, SOURCE_REF) = 0;

		// generic:
		void set_value(const std::string &v, SOURCE_REF);

		// return void instead of Param-based return type as we don't accept any copy/move constructors either!
		void operator=(const char *value);
		void operator=(const std::string &value);

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = 0, SOURCE_TYPE) = 0;
		void ResetToDefault(const ParamsVectorSet &source_vec, SOURCE_TYPE);

		Param(const Param &o) = delete;
		Param(Param &&o) = delete;
		Param(const Param &&o) = delete;

		Param &operator=(const Param &other) = delete;
		Param &operator=(Param &&other) = delete;
		Param &operator=(const Param &&other) = delete;

		ParamType type() const noexcept;

	protected:
		const char *name_; // name of this parameter
		const char *info_; // for menus

		Param *setter_;
		ParamsVector &owner_;

#if 0
		ParamValueContainer value_;
		ParamValueContainer default_;
#endif
		mutable access_counts_t access_counts_;

		ParamType type_ : 13;

		ParamSetBySourceType set_mode_ : 4;

		bool init_ : 1; // needs to be set before first use, i.e. can be set 'during application init phase only'
		bool debug_ : 1;
		bool set_ : 1;
		bool set_to_non_default_value_ : 1;
		bool locked_ : 1;
		bool error_ : 1;
	};

	// --------------------------------------------------------------------------------------------------

#define THE_4_HANDLERS_PROTO                                                                    \
      const char *name, const char *comment, ParamsVector &owner, bool init = false,            \
      ParamOnModifyFunction on_modify_f = 0, ParamOnValidateFunction on_validate_f = 0,         \
      ParamOnParseFunction on_parse_f = 0, ParamOnFormatFunction on_format_f = 0

/*
 * NOTE: a previous version of these typed parameter classes used C++ templates, but I find that templates cannot do one thing:
 * make sure that the copy and move constructors + operator= methods for the *final class* are matched, unless such a class is
 * only a `using` statement of a template instantiation.
 *
 * Hence we succumb to using preprocessor macros below instead, until someone better versed in C++ than me comes along a keeps thing readable; I didn't succeed
 * for the RefTypeParam-based StringSetParam and IntSetParam classes, so those are produced with some help from the preprocessor
 * instead.
 */

 // Using this one as the base for fundamental types:
	template <class T>
	class ValueTypedParam: public Param {
		using RTP = ValueTypedParam<T>;

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
		ValueTypedParam(const T value, THE_4_HANDLERS_PROTO);
		virtual ~ValueTypedParam() = default;

		operator T() const;
		void operator=(const T value);

		virtual void set_value(const char *v, SOURCE_REF) override;
		void set_value(T v, SOURCE_REF);

		T value() const noexcept;

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = 0, SOURCE_TYPE) override;
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
	};

	// --------------------------------------------------------------------------------------------------

	// Use this one for std::String parameters: those are kinda special...
	template <class T>
	class StringTypedParam: public Param {
		using RTP = StringTypedParam<T>;

	public:
		using Param::Param;
		using Param::operator=;

		// Return true when modify action may proceed. `new_value` MAY have been adjusted by this modify handler. The modify handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnModifyCFunction(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnValidateCFunction(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnParseCFunction(RTP &target, T &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string ParamOnFormatCFunction(const RTP &source, const T &value, const T& default_value, ValueFetchPurpose purpose);

		using ParamOnModifyFunction = std::function<ParamOnModifyCFunction>;
		using ParamOnValidateFunction = std::function<ParamOnValidateCFunction>;
		using ParamOnParseFunction = std::function<ParamOnParseCFunction>;
		using ParamOnFormatFunction = std::function<ParamOnFormatCFunction>;

	public:
		StringTypedParam(const char *value, THE_4_HANDLERS_PROTO);
		StringTypedParam(const T &value, THE_4_HANDLERS_PROTO);
		explicit StringTypedParam(const T *value, THE_4_HANDLERS_PROTO);
		virtual ~StringTypedParam() = default;

		// operator T() const;
		operator const T &() const;
		operator const T *() const;
		// void operator=(T value);
		void operator=(const T &value);
		void operator=(const T *value);

		operator const std::string &();
		const char *c_str() const;

		bool empty() const noexcept;

		bool contains(const std::string &sv) const noexcept;
		bool contains(char ch) const noexcept;
		bool contains(const char *s) const noexcept;

		virtual void set_value(const char *v, SOURCE_REF) override;
		void set_value(const T &v, SOURCE_REF);

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

		const T &value() const noexcept;

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = 0, SOURCE_TYPE) override;
		using Param::ResetToDefault;

		virtual std::string value_str(ValueFetchPurpose purpose) const override;

		StringTypedParam(const RTP &o) = delete;
		StringTypedParam(RTP &&o) = delete;
		StringTypedParam(const RTP &&o) = delete;

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
	};

	// --------------------------------------------------------------------------------------------------

	// Use this one for arbitrary other classes of parameter you wish to use/track:
	template <class T, class Assistant>
	class RefTypedParam: public Param {
		using RTP = RefTypedParam<T, Assistant>;

	public:
		using Param::Param;
		using Param::operator=;

		// Return true when modify action may proceed. `new_value` MAY have been adjusted by this modify handler. The modify handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnModifyCFunction(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnValidateCFunction(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnParseCFunction(RTP &target, T &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string ParamOnFormatCFunction(const RTP &source, const T &value, const T& default_value, ValueFetchPurpose purpose);

		using ParamOnModifyFunction = std::function<ParamOnModifyCFunction>;
		using ParamOnValidateFunction = std::function<ParamOnValidateCFunction>;
		using ParamOnParseFunction = std::function<ParamOnParseCFunction>;
		using ParamOnFormatFunction = std::function<ParamOnFormatCFunction>;

	public:
		RefTypedParam(const char *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		RefTypedParam(const T &value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		explicit RefTypedParam(const T *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		virtual ~RefTypedParam();

		//operator T() const;
		operator const T&() const;
		operator const T *() const;
		//void operator=(T value);
		void operator=(const T &value);
		void operator=(const T *value);

		// Produce a reference to the parameter-internal assistant instance.
		// 
		// Used, for example, by the parse handler, to obtain info about delimiters, etc., necessary to successfully parse a string value into a T object.
		Assistant &get_assistant();
		const Assistant &get_assistant() const;

		operator const std::string &();
		const char* c_str() const;

		bool empty() const noexcept;

		virtual void set_value(const char *v, SOURCE_REF) override;
		void set_value(const T &v, SOURCE_REF);

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

		const T &value() const noexcept;

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = 0, SOURCE_TYPE) override;
		using Param::ResetToDefault;

		virtual std::string value_str(ValueFetchPurpose purpose) const override;

		RefTypedParam(const RTP &o) = delete;
		RefTypedParam(RTP &&o) = delete;
		RefTypedParam(const RTP &&o) = delete;

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

	// Use this one for sets (array/vector) of basic types:
	template <class ElemT, class Assistant>
	class BasicVectorTypedParam: public Param {
		using RTP = BasicVectorTypedParam<ElemT, Assistant>;
		using VecT = std::vector<ElemT>;

	public:
		using Param::Param;
		using Param::operator=;

		// Return true when modify action may proceed. `new_value` MAY have been adjusted by this modify handler. The modify handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnModifyCFunction(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnValidateCFunction(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnParseCFunction(RTP &target, VecT &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string ParamOnFormatCFunction(const RTP &source, const VecT &value, const VecT& default_value, ValueFetchPurpose purpose);

		using ParamOnModifyFunction = std::function<ParamOnModifyCFunction>;
		using ParamOnValidateFunction = std::function<ParamOnValidateCFunction>;
		using ParamOnParseFunction = std::function<ParamOnParseCFunction>;
		using ParamOnFormatFunction = std::function<ParamOnFormatCFunction>;

	public:
		BasicVectorTypedParam(const char *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		BasicVectorTypedParam(const VecT &value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		virtual ~BasicVectorTypedParam() = default;

		// operator T() const;
		operator const VecT &() const;
		operator const VecT *() const;
		// void operator=(T value);
		void operator=(const VecT &value);

		// Produce a reference to the parameter-internal assistant instance.
		//
		// Used, for example, by the parse handler, to obtain info about delimiters, etc., necessary to successfully parse a string value into a T object.
		Assistant &get_assistant();
		const Assistant &get_assistant() const;

		operator const std::string &();
		const char *c_str() const;

		bool empty() const noexcept;

		virtual void set_value(const char *v, SOURCE_REF) override;
		void set_value(const VecT& v, SOURCE_REF);

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

		const VecT &value() const noexcept;

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = 0, SOURCE_TYPE) override;
		using Param::ResetToDefault;

		virtual std::string value_str(ValueFetchPurpose purpose) const override;

		BasicVectorTypedParam(const RTP &o) = delete;
		BasicVectorTypedParam(RTP &&o) = delete;
		BasicVectorTypedParam(const RTP &&o) = delete;

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
		VecT value_;
		VecT default_;
		Assistant assistant_;
	};

	// --------------------------------------------------------------------------------------------------

	// Use this one for sets (array/vector) of user-defined / custom types:
	template <class ElemT, class Assistant>
	class ObjectVectorTypedParam: public Param {
		using RTP = ObjectVectorTypedParam<ElemT, Assistant>;
		using VecT = std::vector<ElemT>;

	public:
		using Param::Param;
		using Param::operator=;

		// Return true when modify action may proceed. `new_value` MAY have been adjusted by this modify handler. The modify handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnModifyCFunction(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnValidateCFunction(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void ParamOnParseCFunction(RTP &target, VecT &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string ParamOnFormatCFunction(const RTP &source, const VecT &value, const VecT& default_value, ValueFetchPurpose purpose);

		using ParamOnModifyFunction = std::function<ParamOnModifyCFunction>;
		using ParamOnValidateFunction = std::function<ParamOnValidateCFunction>;
		using ParamOnParseFunction = std::function<ParamOnParseCFunction>;
		using ParamOnFormatFunction = std::function<ParamOnFormatCFunction>;

	public:
		ObjectVectorTypedParam(const char *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		ObjectVectorTypedParam(const VecT &value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		virtual ~ObjectVectorTypedParam();

		// operator T() const;
		operator const VecT &() const;
		operator const VecT *() const;
		// void operator=(T value);
		void operator=(const VecT &value);
		void operator=(const VecT *value);

		// Produce a reference to the parameter-internal assistant instance.
		//
		// Used, for example, by the parse handler, to obtain info about delimiters, etc., necessary to successfully parse a string value into a T object.
		Assistant &get_assistant();
		const Assistant &get_assistant() const;

		operator const std::string &();
		const char *c_str() const;

		bool empty() const noexcept;

		virtual void set_value(const char *v, SOURCE_REF) override;
		void set_value(const VecT &v, SOURCE_REF);

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

		const VecT &value() const noexcept;

		// Optionally the `source_vec` can be used to source the value to reset the parameter to.
		// When no source vector is specified, or when the source vector does not specify this
		// particular parameter, then its value is reset to the default value which was
		// specified earlier in its constructor.
		virtual void ResetToDefault(const ParamsVectorSet *source_vec = 0, SOURCE_TYPE) override;
		using Param::ResetToDefault;

		virtual std::string value_str(ValueFetchPurpose purpose) const override;

		ObjectVectorTypedParam(const RTP &o) = delete;
		ObjectVectorTypedParam(RTP &&o) = delete;
		ObjectVectorTypedParam(const RTP &&o) = delete;

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
		VecT value_;
		VecT default_;
		Assistant assistant_;
	};

	// --------------------------------------------------------------------------------------------------

#undef THE_4_HANDLERS_PROTO

// see note above: these must be using statements, not derived classes, or otherwise the constructor/operator delete instructions in that base template won't deliver as expected!

	using IntParam = ValueTypedParam<int32_t>;
	using BoolParam = ValueTypedParam<bool>;
	using DoubleParam = ValueTypedParam<double>;

	using StringParam = StringTypedParam<std::string>;


	struct BasicVectorParamParseAssistant {
		std::string parse_separators{"\t\r\n,;:|"}; //< list of separators accepted by the string parse handler. Any one of these separates individual elements in the array.

		// For formatting the set for data serialzation / save purposes, the generated set may be wrapped in a prefix and postfix, e.g. "{" and "}".
		std::string fmt_data_prefix{""};
		std::string fmt_data_postfix{""};
		std::string fmt_data_separator{","};

		// For formatting the set for display purposes, the generated set may be wrapped in a prefix and postfix, e.g. "[" and "]".
		std::string fmt_display_prefix{"["};
		std::string fmt_display_postfix{"]"};
		std::string fmt_display_separator{", "};

		bool parse_should_cope_with_fmt_display_prefixes{true}; //< when true, the registered string parse handler is supposed to be able to cope with encountering the format display-output prefix and prefix strings.
		bool parse_trims_surrounding_whitespace{true};  //< the string parse handler will trim any whitespace occurring before or after every value stored in the string.
	};

	using StringSetParam = BasicVectorTypedParam<std::string, BasicVectorParamParseAssistant>;
	using IntSetParam = BasicVectorTypedParam<int32_t, BasicVectorParamParseAssistant>;
	using BoolSetParam = BasicVectorTypedParam<bool, BasicVectorParamParseAssistant>;
	using DoubleSetParam = BasicVectorTypedParam<double, BasicVectorParamParseAssistant>;

	// --------------------------------------------------------------------------------------------------

	// remove the macros to help set up the member prototypes

#include <parameters/sourceref_defend.h>

} // namespace 

#endif
