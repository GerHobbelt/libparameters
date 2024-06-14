/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_PARAMS_H_
#define _LIB_PARAMS_PARAMS_H_

#include <tesseract/export.h> // for TESS_API

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <unordered_map>
#include <memory>


namespace parameters {

	class Param;
	class TFile;

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

	// Identifiers used to indicate the *origin* of the current parameter value. Used for reporting/diagnostic purposes. Do not treat these
	// as gospel; these are often assigned under limited/reduced information conditions, so they merely serve as report *hints*.
	enum ParamSetBySourceType {
		PARAM_VALUE_IS_DEFAULT = 0,

		PARAM_VALUE_IS_RESET,
		PARAM_VALUE_IS_SET_BY_ASSIGN,			    // 'indirect' write: value is copied over from elsewhere via operator=.
		PARAM_VALUE_IS_SET_BY_PARAM,          // 'indirect' write: other Param's OnChange code set the param value, whatever it is now.
		PARAM_VALUE_IS_SET_BY_PRESET,         // 'indirect' write: a tesseract 'preset' parameter set was invoked and that one set this one as part of the action.
		PARAM_VALUE_IS_SET_BY_CORE_RUN,       // 'explicit' write by the running tesseract core: before proceding with the next step the run-time adjusts this one, e.g. (incrementing) page number while processing a multi-page OCR run.
		PARAM_VALUE_IS_SET_BY_CONFIGFILE,     // 'explicit' write by loading and processing a config file.
		PARAM_VALUE_IS_SET_BY_APPLICATION,    // 'explicit' write: user / application code set the param value, whatever it is now.
	};

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
	void set_current_application_default_param_source_type(ParamSetBySourceType source_type);

	/**
	 * Produces the current default application source type; intended to be used internally by our parameters support library code.
	 */
	ParamSetBySourceType get_current_application_default_param_source_type();

	// --------------------------------------------------------------------------------------------------

// Custom equivalent of std::hash<Param> + std::equal_to<Param> for std::unordered_map<const char *key, Param & value>.
	class ParamHash
	{
	public:
		// hash:
		std::size_t operator()(const Param& s) const noexcept;
		std::size_t operator()(const char * s) const noexcept;

		// equal_to:
		bool operator()(const Param& lhs, const Param& rhs) const noexcept;
		// equal_to:
		bool operator()(const char * lhs, const char * rhs) const noexcept;
	};

	// --------------------------------------------------------------------------------------------------

	// Custom implementation of class Compare for std::sort.
	class ParamComparer
	{
	public:
		// compare as a-less-b? for purposes of std::sort et al:
		bool operator()(const Param& lhs, const Param& rhs) const;
		// compare as a-less-b? for purposes of std::sort et al:
		bool operator()(const char * lhs, const char * rhs) const;
	};

	// --------------------------------------------------------------------------------------------------

	// Readability helper types: reference and pointer to `Param` base class.
	typedef Param & ParamRef;
	typedef Param * ParamPtr;


	// Readability helper: reduce noise by using macros to help set up the member prototypes.

#define SOURCE_TYPE																                        \
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION

#define SOURCE_REF																                        \
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION,	\
		ParamPtr source = nullptr

// Hash table used internally as a database table, collecting the compiled-in parameter instances.
// Used to speed up the various `find()` functions, among others.
	typedef std::unordered_map<
		const char * /* key */,
		ParamPtr /* value */,
		ParamHash /* hash value calc */,
		ParamHash /* equality check */
	> ParamsHashTableType;

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
	class ParamsVector {
	private:
		ParamsHashTableType params_;
		bool is_params_owner_ = false;
		std::string title_;

	public:
		ParamsVector() = delete;
		ParamsVector(const char* title);
		ParamsVector(const char *title, std::initializer_list<ParamPtr> vecs);

		~ParamsVector();

		// **Warning**: this method may only safely be invoked *before* any parameters have been added to the set.
		//
		// Signal the ParamVector internals that this particular set is *parameter instances owner*, i.e. all parameters added to
		// this set also transfer their ownership alongside: while the usual/regular ParamVector only stores *references*
		// to parameter instances owned by *others*, now we will be owner of those parameter instances and thus responsible for heap memory cleanup
		// once our ParamVector destructor is invoked.
		//
		// This feature is used, f.e., in tesseract when the command line parser collects the
		// user-specified parameter values from the command line itself and various referenced or otherwise
		// implicated configuration files: a 'muster set' of known compiled-in parameters is collected and cloned into such
		// a 'owning' parameter vector, which is then passed on to the command line parser proper to use for both parameter name/type/value
		// validations while parsing and storing the parsed values discovered.
		// 
		// It is used to collect, set up and then pass parameter vectors into the tesseract Init* instance
		// methods: by using a (cloned and) *owning* parameter vector, we can simply collect and pass any configuration parameters
		// we wish to have adjusted into the tesseract instance and have these 'activated' only then, i.e. we won't risk
		// modifying any *live* parameters while working on/with the vector-owned set.
		void mark_as_all_params_owner();

		void add(ParamPtr param_ref);
		void add(ParamRef param_ref);
		void add(std::initializer_list<ParamPtr> vecs);

		void remove(ParamPtr param_ref);
		void remove(ParamRef param_ref);

		const char* title() const;
		void change_title(const char* title);

		ParamPtr find(
			const char *name,
			ParamType accepted_types_mask
		) const;

		template <ParamDerivativeType T>
		T *find(
			const char *name
		) const;

		std::vector<ParamPtr> as_list(
			ParamType accepted_types_mask = ANY_TYPE_PARAM
		) const;

		friend class ParamsVectorSet;
	};

	// --------------------------------------------------------------------------------------------------

	// an (ad-hoc?) collection of ParamsVector instances.
	class ParamsVectorSet final {
	private:
		std::vector<ParamsVector *> collection_;

	public:
		ParamsVectorSet();
		ParamsVectorSet(std::initializer_list<ParamsVector *> vecs);

		~ParamsVectorSet();

		void add(ParamsVector &vec_ref);
		void add(ParamsVector *vec_ref);
		void add(std::initializer_list<ParamsVector *> vecs);

		ParamPtr find(
			const char *name,
			ParamType accepted_types_mask
		) const;

		template <ParamDerivativeType T>
		T *find(
			const char *name
		) const;

		std::vector<ParamPtr> as_list(
			ParamType accepted_types_mask = ANY_TYPE_PARAM
		) const;

		ParamsVector flattened_copy(ParamType accepted_types_mask = ANY_TYPE_PARAM) const;
	};

	// --------------------------------------------------------------------------------------------------

	// A simple FILE/stdio wrapper class which supports reading from stdin or regular file.
	class ConfigFile {
	public:
		// Parse '-', 'stdin' and '1' as STDIN, or open a regular text file in UTF8 read mode.
		//
		// An error line is printed via `tprintf()` when the given path turns out not to be valid.
		ConfigFile(const char *path);
		ConfigFile(const std::string &path)
			: ConfigFile(path.c_str()) {}
		~ConfigFile();

		FILE *operator()() const;

		operator bool() const {
			return _f != nullptr;
		};

	private:
		FILE *_f;
	};

	// The very first time we call this one during the current run, we CREATE/OVERWRITE the target file.
	// Iff we happen to invoke this call multiple times for the same target file, any subsequent call
	// will APPEND to the target file.
	//
	// NOTE: as we need to track the given file path, this will incur a very minor heap memory leak
	// as we won't ever release the memory allocated for that string in `_processed_file_paths`.
	class ReportFile {
	public:
		// Parse '-', 'stdout' and '1' as STDIN, '+', 'stderr', and '2' as STDERR, or open a regular text file in UTF8 write mode.
		//
		// MAY return NULL, e.g. when path is empty. This is considered a legal/valid use & behaviour.
		// On the other hand, an error line is printed via `tprintf()` when the given path is non-empty and
		// turns out not to be valid.
		// Either way, a return value of NULL implies that default behaviour (output via `tprintf()`) is assumed.
		ReportFile(const char *path);
		ReportFile(const std::string &path): ReportFile(path.c_str()) {}
		~ReportFile();

		FILE * operator()() const;

		operator bool() const {
			return _f != nullptr;
		};

	private:
		FILE *_f;

		// We assume we won't be processing very many file paths through here, so a linear scan through
		// the set-processed-to-date is considered good enough/best re performance, hence std::vector 
		// sufices for the tracking list.
		static std::vector<std::string> _processed_file_paths;
	};


	// --------------------------------------------------------------------------------------------------

	// Definition of various parameter types.
	class Param {
	protected:
		Param(const char *name, const char *comment, ParamsVector &owner, bool init = false);

	public:
		virtual ~Param() = default;

		const char *name_str() const noexcept;
		const char *info_str() const noexcept;
		bool is_init() const noexcept;
		bool is_debug() const noexcept;
		bool is_set() const noexcept;
		bool is_set_to_non_default_value() const noexcept;
		bool is_locked() const noexcept;
		bool has_faulted() const noexcept;

		void lock(bool locking = true);

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
			uint16_t faulting;

			// the sum of the previous section's counts: the collected history from previous runs during this application life time.
			uint16_t prev_sum_reading;
			uint16_t prev_sum_writing;
			uint16_t prev_sum_changing;
			uint16_t prev_sum_faulting;
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

			// Fetches the (raw, parseble for re-use via set_value() or storing to serialized text data format files) value of the param as a string and DOES add
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
			// We do not count this read access as this method is for *display* purposes only and we do not
			// wish to tally those together with the actual work code accessing this parameter through
			// the other functions: set_value() and assignment operators.
			VALSTR_PURPOSE_TYPE_INFO,
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
		typedef void (*ParamOnModifyFunction)(RTP &target, const T old_value, T &new_value, const T default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return when validation action passed and modify/write may proceed; throw an exception on (non-recovered) error. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data. Minor infractions (which resulted in some form of recovery) may be signaled by flagging the parameter state via its fault() API method.
		typedef void (*ParamOnValidateFunction)(RTP &target, const T old_value, T &new_value, const T default_value, ParamSetBySourceType source_type);

		// Return when the parse action (parsing `source_value_str` starting at offset `pos`) completed successfully or required only minor recovery; throw an exception on (non-recovered) error.
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		// Minor infractions (which resulted in some form of recovery) may be signaled by flagging the parameter state via its fault() API method.
		typedef void (*ParamOnParseFunction)(RTP& target, T& new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		// This formatting action is supposed to always succeed or fail fatally (e.g. out of heap memory) by throwing an exception.
		// The formatter implementation is not supposed to signal any errors via the fault() API method.
		typedef std::string (*ParamOnFormatFunction)(const RTP &source, const T value, const T default_value, ValueFetchPurpose purpose);

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
		typedef void (*ParamOnModifyFunction)(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnValidateFunction)(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnParseFunction)(RTP &target, T &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string (*ParamOnFormatFunction)(const RTP &source, const T &value, const T& default_value, ValueFetchPurpose purpose);

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
		typedef void (*ParamOnModifyFunction)(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnValidateFunction)(RTP &target, const T &old_value, T &new_value, const T &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnParseFunction)(RTP &target, T &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string (*ParamOnFormatFunction)(const RTP &source, const T &value, const T& default_value, ValueFetchPurpose purpose);

	public:
		RefTypedParam(const char *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		RefTypedParam(const T &value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		explicit RefTypedParam(const T *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		virtual ~RefTypedParam() = default;

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
		typedef void (*ParamOnModifyFunction)(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnValidateFunction)(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnParseFunction)(RTP &target, VecT &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string (*ParamOnFormatFunction)(const RTP &source, const VecT &value, const VecT& default_value, ValueFetchPurpose purpose);

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
		typedef void (*ParamOnModifyFunction)(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type, ParamPtr optional_setter);

		// Return true when validation action passed and modify/write may proceed. `new_value` MAY have been adjusted by this validation handler. The validation handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnValidateFunction)(RTP &target, const VecT &old_value, VecT &new_value, const VecT &default_value, ParamSetBySourceType source_type);

		// Return true when parse action succeeded (parsing `source_value_str` starting at offset `pos`).
		// `new_value` will contain the parsed value produced by this parse handler, while `pos` will have been moved to the end of the parsed content.
		// The string parse handler is not supposed to modify any read/write/modify access accounting data.
		typedef void (*ParamOnParseFunction)(RTP &target, VecT &new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type);

		// Return the formatted string value, depending on the formatting purpose. The format handler is not supposed to modify any read/write/modify access accounting data.
		typedef std::string (*ParamOnFormatFunction)(const RTP &source, const VecT &value, const VecT& default_value, ValueFetchPurpose purpose);

	public:
		ObjectVectorTypedParam(const char *value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		ObjectVectorTypedParam(const VecT &value, const Assistant &assist, THE_4_HANDLERS_PROTO);
		virtual ~ObjectVectorTypedParam() = default;

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

	// Utility functions for working with Tesseract parameters.
	class TESS_API ParamUtils {
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
															 SOURCE_REF);

		// Read parameters from the given file pointer.
		// Otherwise identical to ReadParamsFile().
		static bool ReadParamsFromFp(TFile *fp,
																 const ParamsVectorSet &set,
																 SOURCE_REF);

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

#if 0
		// Fetches the value of the named param as a string and does not add
		// this access to the read counter tally. This is useful, f.e., when printing 'init'
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		// Returns false if not found. Prints a message via `tprintf()` to report this fact
		// (see also `FindParam()`).
		//
		// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
		static bool InspectParamAsString(
				std::string *value_ref, const char *name,
				const ParamsVectorSet &set,
				ParamType accepted_types_mask = ANY_TYPE_PARAM);

		// Fetches the value of the named param as a string and does not add
		// this access to the read counter tally. This is useful, f.e., when printing 'init'
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process.
		// Returns false if not found. Prints a message via `tprintf()` to report this fact
		// (see also `FindParam()`).
		//
		// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
		static bool InspectParamAsString(
				std::string *value_ref, const char *name,
				const ParamsVector &set,
				ParamType accepted_types_mask = ANY_TYPE_PARAM);

		// Fetches the value of the named param as a ParamValueContainer and does not add
		// this access to the read counter tally. This is useful, f.e., when editing 'init'
		// (only-settable-before-first-use) parameters in a UI before starting the actual
		// process.
		// Returns false if not found. Prints a message via `tprintf()` to report this
		// fact (see also `FindParam()`).
		//
		// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
		static bool InspectParam(
				std::string &value_dst, const char *name,
				const ParamsVectorSet &set,
				ParamType accepted_types_mask = ANY_TYPE_PARAM);
#endif

		// Print all parameters in the given set(s) to the given file.
		static void PrintParams(FILE *fp, const ParamsVectorSet &set, bool print_info = true);

		// Report parameters' usage statistics, i.e. report which params have been
		// set, modified and read/checked until now during this run-time's lifetime.
		//
		// Use this method for run-time 'discovery' about which tesseract parameters
		// are actually *used* during your particular usage of the library, ergo
		// answering the question:
		// "Which of all those parameters are actually *relevant* to my use case today?"
		//
		// When `section_title` is NULL, this will report the lump sum parameter usage
		// for the entire run. When `section_title` is NOT NULL, this will only report
		// the parameters that were actually used (R/W) during the last section of the
		// run, i.e. since the previous invocation of this reporting method (or when
		// it hasn't been called before: the start of the application).
		//
		// Unless `f` is stdout/stderr, this method reports via `tprintf()` as well.
		// When `f` is a valid handle, then the report is written to the given FILE,
		// which may be stdout/stderr.
		//
		// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
		static void ReportParamsUsageStatistics(FILE *fp, const ParamsVectorSet &set, const char *section_title = nullptr);

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

#undef SOURCE_TYPE															
#undef SOURCE_REF																

// --------------------------------------------------------------------------------------------------




// Global parameter lists.
//
// To avoid the problem of undetermined order of static initialization
// global_params are accessed through the GlobalParams function that
// initializes the static pointer to global_params only on the first time
// GlobalParams() is called.
//
// TODO(daria): remove GlobalParams() when all global Tesseract
// parameters are converted to members.
	TESS_API
		ParamsVector &GlobalParams();


	/*************************************************************************
	 * Note on defining parameters.
	 *
	 * The values of the parameters defined with *_INIT_* macros are guaranteed
	 * to be loaded from config files before Tesseract initialization is done
	 * (there is no such guarantee for parameters defined with the other macros).
	 *************************************************************************/

#define INT_VAR_H(name) ::tesseract::IntParam name

#define BOOL_VAR_H(name) ::tesseract::BoolParam name

#define STRING_VAR_H(name) ::tesseract::StringParam name

#define DOUBLE_VAR_H(name) ::tesseract::DoubleParam name

#define INT_VAR(name, val, comment) \
  ::tesseract::IntParam name(val, #name, comment, ::tesseract::GlobalParams())

#define BOOL_VAR(name, val, comment) \
  ::tesseract::BoolParam name(val, #name, comment, ::tesseract::GlobalParams())

#define STRING_VAR(name, val, comment) \
  ::tesseract::StringParam name(val, #name, comment, ::tesseract::GlobalParams())

#define DOUBLE_VAR(name, val, comment) \
  ::tesseract::DoubleParam name(val, #name, comment, ::tesseract::GlobalParams())

#define INT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define BOOL_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define STRING_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define DOUBLE_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define INT_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define BOOL_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define STRING_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define DOUBLE_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)


} // namespace 

#endif
