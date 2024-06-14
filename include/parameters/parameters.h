/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_PARAMS_H_
#define _LIB_PARAMS_PARAMS_H_

#include "mupdf/assertions.h" // for ASSERT

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <variant>
#include <functional>
#include <unordered_map>


namespace parameters {

	class Param;
	class IntParam;
	class BoolParam;
	class StringParam;
	class DoubleParam;
	class TFile;


	enum ParamType {
		UNKNOWN_PARAM = 0,

		INT_PARAM = 1,
		BOOL_PARAM = 2,
		DOUBLE_PARAM = 4,
		STRING_PARAM = 8,

		ANY_TYPE_PARAM = 15,
	};

	enum ParamSetBySourceType {
		PARAM_VALUE_IS_DEFAULT = 0,

		PARAM_VALUE_IS_RESET,
		PARAM_VALUE_IS_SET_BY_ASSIGN,			// 'indirect' write: value is copied over from elsewhere via operator=.
		PARAM_VALUE_IS_SET_BY_PARAM,			// 'indirect' write: other Param's OnChange code set the param value, whatever it is now.
		PARAM_VALUE_IS_SET_BY_APPLICATION,		// 'explicit' write: user / application code set the param value, whatever it is now.
	};


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

	// Custom implementation of class Compare for std::sort.
	class ParamComparer
	{
	public:
		// compare as a-less-b? for purposes of std::sort et al:
		bool operator()(const Param& lhs, const Param& rhs) const;
		// compare as a-less-b? for purposes of std::sort et al:
		bool operator()(const char * lhs, const char * rhs) const;
	};

	typedef Param & ParamRef;
	typedef Param * ParamPtr;


	// reduce noise by using macros to help set up the member prototypes

#define SOURCE_TYPE																\
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION

#define SOURCE_REF																\
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION,	\
		ParamPtr source = nullptr


	typedef std::unordered_map<
		const char * /* key */,
		ParamPtr /* value */,
		ParamHash /* hash value calc */,
		ParamHash /* equality check */
	> ParamsHashTableType;


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

	static_assert(ParamAcceptableValueType<int>);
	static_assert(ParamAcceptableValueType<double>);
	static_assert(ParamAcceptableValueType<bool>);
	static_assert(!ParamAcceptableValueType<const char *>);
	static_assert(!ParamAcceptableValueType<char*>);
	static_assert(!ParamAcceptableValueType<std::string>);
	static_assert(!ParamAcceptableValueType<std::string&>);

	//template<typename T>
	//concept ParamDerivativeType = std::is_base_of<Param, T>::value;

#define ParamDerivativeType   typename

#else

#define ParamAcceptableValueType   class

#define ParamDerivativeType   class

#endif


	class ParamsVector {
	private:
		ParamsHashTableType params_;
		std::string title_;

	public:
		ParamsVector() = delete;
		ParamsVector(const char* title);
		ParamsVector(const char *title, std::initializer_list<ParamPtr> vecs);

		~ParamsVector();

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


	// an (ad-hoc?) collection of ParamsVector instances.
	class ParamsVectorSet {
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
	};

	// ready-made template instances:
#if 01
	template <>
	IntParam* ParamsVectorSet::find<IntParam>(
		const char* name
	) const;
	template <>
	BoolParam* ParamsVectorSet::find<BoolParam>(
		const char* name
	) const;
	template <>
	DoubleParam* ParamsVectorSet::find<DoubleParam>(
		const char* name
	) const;
	template <>
	StringParam* ParamsVectorSet::find<StringParam>(
		const char* name
	) const;
	template <>
	Param* ParamsVectorSet::find<Param>(
		const char* name
	) const;
#endif
	//--------------------


	class Param;

	typedef std::variant<bool, int, double, std::string> ParamValueContainer;

	typedef std::function<void (
		const char * /* name */,
		ParamRef /* target */,
		ParamSetBySourceType /* source type */,
		ParamPtr /* optional setter/parent */,
		ParamValueContainer & /* old value */,
		ParamValueContainer & /* new value */
	)> ParamOnModifyFunction;


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
		//
		// Any Variables listed in the file which do not match the given
		// constraint are ignored, but are reported via `tprintf()` as ignored,
		// unless you set `quietly_ignore`.
		static bool ReadParamsFile(const std::string &file, // filename to read
			const ParamsVectorSet &set,
			SOURCE_REF,
			bool quietly_ignore = false
		);

		// Read parameters from the given file pointer.
		// Otherwise identical to ReadParamsFile().
		static bool ReadParamsFromFp(TFile *fp,
			const ParamsVectorSet &set,
			SOURCE_REF,
			bool quietly_ignore = false
		);

		// Set a parameter to have the given value.
		//
		// A Variable, which does not match the given constraint, is ignored, 
		// but is reported via `tprintf()` as ignored,
		// unless you set `quietly_ignore`.
		template <ParamAcceptableValueType T>
		static bool SetParam(
			const char *name, const T value,
			const ParamsVectorSet &set,
			SOURCE_REF,
			bool quietly_ignore = false
		);
		static bool SetParam(
			const char* name, const char *value,
			const ParamsVectorSet& set,
			SOURCE_REF,
			bool quietly_ignore = false
		);
		static bool SetParam(
			const char* name, const std::string& value,
			const ParamsVectorSet& set,
			SOURCE_REF,
			bool quietly_ignore = false
		);

		// Set a parameter to have the given value.
		//
		// A Variable, which does not match the given constraint, is ignored, 
		// but is reported via `tprintf()` as ignored,
		// unless you set `quietly_ignore`.
		template <ParamAcceptableValueType T>
		static bool SetParam(
			const char *name, const T value,
			ParamsVector &set,
			SOURCE_REF,
			bool quietly_ignore = false
		);
		static bool SetParam(
			const char* name, const char* value,
			ParamsVector& set,
			SOURCE_REF,
			bool quietly_ignore = false
		);
		static bool SetParam(
			const char* name, const std::string& value,
			ParamsVector& set,
			SOURCE_REF,
			bool quietly_ignore = false
		);

		// Produces a pointer (reference) to the parameter with the given name (of the
		// appropriate type) if it was found in any of the given vectors.
		// When `set` is empty, the `GlobalParams()` vector will be assumed 
		// instead.
		//
		// Returns nullptr when param is not known.
		template <ParamDerivativeType T>
		static T *FindParam(
			const char *name,
			const ParamsVectorSet &set
		);
		static Param* FindParam(
			const char* name,
			const ParamsVectorSet& set,
			ParamType accepted_types_mask = ANY_TYPE_PARAM
		);

		// Produces a pointer (reference) to the parameter with the given name (of the
		// appropriate type) if it was found in any of the given vectors.
		// When `set` is empty, the `GlobalParams()` vector will be assumed
		// instead.
		//
		// Returns nullptr when param is not known.
		template <ParamDerivativeType T>
		static T *FindParam(
			const char *name,
			const ParamsVector &set
		);
		static Param* FindParam(
			const char* name,
			const ParamsVector& set,
			ParamType accepted_types_mask = ANY_TYPE_PARAM
		);

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
			ParamType accepted_types_mask = ANY_TYPE_PARAM,
			bool quietly_ignore = false
		);

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
			ParamType accepted_types_mask = ANY_TYPE_PARAM,
			bool quietly_ignore = false
		);

		// Fetches the value of the named param as a ParamValueContainer and does not add 
		// this access to the read counter tally. This is useful, f.e., when editing 'init' 
		// (only-settable-before-first-use) parameters in a UI before starting the actual 
		// process.
		// Returns false if not found. Prints a message via `tprintf()` to report this 
		// fact (see also `FindParam()`).
		//
		// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
		static bool InspectParam(
			ParamValueContainer &value_dst, const char *name,
			const ParamsVectorSet &set,
			ParamType accepted_types_mask = ANY_TYPE_PARAM,
			bool quietly_ignore = false
		);

		// Fetches the value of the named param as a ParamValueContainer and does not add 
		// this access to the read counter tally. This is useful, f.e., when editing 'init' 
		// (only-settable-before-first-use) parameters in a UI before starting the actual 
		// process.
		// Returns false if not found. Prints a message via `tprintf()` to report this 
		// fact (see also `FindParam()`).
		//
		// When `set` is empty, the `GlobalParams()` vector will be assumed instead.
		static bool InspectParam(
			ParamValueContainer &value_dst, const char *name,
			const ParamsVector &set,
			ParamType accepted_types_mask = ANY_TYPE_PARAM,
			bool quietly_ignore = false
		);

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
#if 01
	template <>
	IntParam* ParamUtils::FindParam<IntParam>(
			const char* name,
			const ParamsVectorSet& set
	);
	template <>
	BoolParam* ParamUtils::FindParam<BoolParam>(
			const char* name,
			const ParamsVectorSet& set
	);
	template <>
	DoubleParam* ParamUtils::FindParam<DoubleParam>(
			const char* name,
			const ParamsVectorSet& set
	);
	template <>
	StringParam* ParamUtils::FindParam<StringParam>(
			const char* name,
			const ParamsVectorSet& set
	);
	template <>
	Param* ParamUtils::FindParam<Param>(
			const char* name,
			const ParamsVectorSet& set
	);
	template <ParamDerivativeType T>
	T* ParamUtils::FindParam(
		const char* name,
		const ParamsVector& set
	) {
		ParamsVectorSet pvec({const_cast<ParamsVector*>(&set)});

		return FindParam<T>(
			name,
			pvec
		);
	}
	template <>
	bool ParamUtils::SetParam<int32_t>(
			const char* name, const int32_t value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source,
			bool quietly_ignore
	);
	template <>
	bool ParamUtils::SetParam<bool>(
			const char* name, const bool value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source,
			bool quietly_ignore
	);
	template <>
	bool ParamUtils::SetParam<double>(
			const char* name, const double value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source,
			bool quietly_ignore
	);
	template <ParamAcceptableValueType T>
	bool ParamUtils::SetParam(
		const char* name, const T value,
		ParamsVector& set,
		ParamSetBySourceType source_type, ParamPtr source,
		bool quietly_ignore
	) {
		ParamsVectorSet pvec({&set});
		return SetParam<T>(name, value, pvec, source_type, source, quietly_ignore);
	}
#endif
	//--------------------

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


	// Definition of various parameter types.
	class Param {
	public:
		virtual ~Param() = default;

		const char* name_str() const;
		const char* info_str() const;
		bool is_init() const;
		bool is_debug() const;

		ParamSetBySourceType set_mode() const;
		Param* is_set_by() const;

		ParamsVector& owner() const;

		// We track Param/Variable setup/changes/usage through this administrative struct.
		// It helps us to diagnose and report which tesseract Params (Variables) are actually
		// USED in which program section and in the program as a whole, while we can also 
		// diagnose which variables have been set up during this session, and by *whom*, as
		// some Params will have been modified due to others having been set, e.g. `debug_all`.
		typedef struct access_counts {
			// the current section's counts
			int reading;
			int writing;
			int changing;

			// the sum of the previous section's counts
			int prev_sum_reading;
			int prev_sum_writing;
			int prev_sum_changing;
		} access_counts_t;

		access_counts_t access_counts() const;

		void reset_access_counts();

		// Fetches the (formatted for print/display) value of the param as a string and does not add 
		// this access to the read counter tally. This is useful, f.e., when printing 'init' 
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process. 
		virtual std::string formatted_value_str() const = 0;

		// Fetches the (raw, parseble for re-use via set_value()) value of the param as a string and does not add 
		// this access to the read counter tally. This is useful, f.e., when printing 'init' 
		// (only-settable-before-first-use) parameters to config file or log file, independent
		// from the actual work process. 
		virtual std::string raw_value_str() const = 0;

		// Return string representing the type of the parameter value, e.g. "integer"
		virtual const char *value_type_str() const = 0;

		// Fetches the value of the param and delivers it in a ParamValueContainer union. 
		// Does not add this access to the read counter tally. This is useful, f.e., when 
		// editing 'init' (only-settable-before-first-use) parameters in a UI before starting
		// the actual work process.
		virtual bool inspect_value(ParamValueContainer &dst) const = 0;

		virtual bool set_value(const char *v, SOURCE_REF) = 0;
		virtual bool set_value(int32_t v, SOURCE_REF) = 0;
		virtual bool set_value(bool v, SOURCE_REF) = 0;
		virtual bool set_value(double v, SOURCE_REF) = 0;

		// generic:
		bool set_value(const ParamValueContainer &v, SOURCE_REF);
		bool set_value(const std::string &v, SOURCE_REF);

		virtual void ResetToDefault(SOURCE_TYPE) = 0;
		virtual void ResetFrom(const ParamsVectorSet &vec, SOURCE_TYPE) = 0;

		Param(const Param &o) = delete;
		Param(Param &&o) = delete;
		Param(const Param &&o) = delete;

		Param &operator=(const Param &other) = delete;
		Param &operator=(Param &&other) = delete;
		Param &operator=(const Param &&other) = delete;

		ParamType type() const;

		ParamOnModifyFunction set_on_modify_handler(ParamOnModifyFunction on_modify_f);
		ParamOnModifyFunction clear_on_modify_handler();

	protected:
		Param(const char* name, const char* comment, ParamsVector& owner, bool init = false, ParamOnModifyFunction on_modify_f = 0);

	protected:
		const char *name_; // name of this parameter
		const char *info_; // for menus

		ParamOnModifyFunction on_modify_f_;

#if 0
		ParamValueContainer value_;
		ParamValueContainer default_;
#endif
		ParamType type_;

		ParamSetBySourceType set_mode_;
		Param *setter_;
		ParamsVector &owner_;

		bool init_;        // needs to be set before first use, i.e. can be set 'during application init phase only' 
		bool debug_;

		mutable access_counts_t access_counts_;
	};


	class IntParam: public Param {
	public:
		IntParam(int32_t value, const char* name, const char* comment, ParamsVector& owner, bool init = false, ParamOnModifyFunction on_modify_f = 0);
		virtual ~IntParam() = default;

		operator int32_t() const;
		void operator=(int32_t value);

		virtual bool set_value(const char *v, SOURCE_REF) override;
		virtual bool set_value(int32_t v, SOURCE_REF) override;
		virtual bool set_value(bool v, SOURCE_REF) override;
		virtual bool set_value(double v, SOURCE_REF) override;

		int32_t value() const;

		virtual void ResetToDefault(SOURCE_TYPE) override;
		virtual void ResetFrom(const ParamsVectorSet& vec, SOURCE_TYPE) override;

		virtual std::string formatted_value_str() const override;
		virtual std::string raw_value_str() const override;
		virtual const char* value_type_str() const override;

		virtual bool inspect_value(ParamValueContainer &dst) const override;

		IntParam(const IntParam &o) = delete;
		IntParam(IntParam &&o) = delete;
		IntParam(const IntParam &&o) = delete;

		IntParam &operator=(const IntParam &other) = delete;
		IntParam &operator=(IntParam &&other) = delete;
		IntParam &operator=(const IntParam &&other) = delete;

	private:
		int32_t value_;
		int32_t default_;
	};

	class BoolParam: public Param {
	public:
		BoolParam(bool value, const char* name, const char* comment, ParamsVector& owner, bool init = false, ParamOnModifyFunction on_modify_f = 0);
		virtual ~BoolParam() = default;

		operator bool() const;
		void operator=(bool value);

		virtual bool set_value(const char *v, SOURCE_REF) override;
		virtual bool set_value(int32_t v, SOURCE_REF) override;
		virtual bool set_value(bool v, SOURCE_REF) override;
		virtual bool set_value(double v, SOURCE_REF) override;

		bool value() const;

		virtual void ResetToDefault(SOURCE_TYPE) override;
		virtual void ResetFrom(const ParamsVectorSet& vec, SOURCE_TYPE) override;

		virtual std::string formatted_value_str() const override;
		virtual std::string raw_value_str() const override;
		virtual const char* value_type_str() const override;

		virtual bool inspect_value(ParamValueContainer &dst) const override;

		BoolParam(const BoolParam &o) = delete;
		BoolParam(BoolParam &&o) = delete;
		BoolParam(const BoolParam &&o) = delete;

		BoolParam &operator=(const BoolParam &other) = delete;
		BoolParam &operator=(BoolParam &&other) = delete;
		BoolParam &operator=(const BoolParam &&other) = delete;

	private:
		bool value_;
		bool default_;
	};

	class StringParam: public Param {
	public:
		StringParam(const char* value, const char* name, const char* comment, ParamsVector& owner, bool init = false, ParamOnModifyFunction on_modify_f = 0);
		virtual ~StringParam() = default;

		operator std::string& ();
		const char* c_str() const;

		bool contains(char c) const;
		bool empty() const;

		bool operator==(const std::string& other) const;
		void operator=(const std::string& value);

		virtual bool set_value(const char *v, SOURCE_REF) override;
		virtual bool set_value(int32_t v, SOURCE_REF) override;
		virtual bool set_value(bool v, SOURCE_REF) override;
		virtual bool set_value(double v, SOURCE_REF) override;

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

		const std::string &value() const;

		virtual void ResetToDefault(SOURCE_TYPE) override;
		virtual void ResetFrom(const ParamsVectorSet& vec, SOURCE_TYPE) override;

		virtual std::string formatted_value_str() const override;
		virtual std::string raw_value_str() const override;
		virtual const char* value_type_str() const override;

		virtual bool inspect_value(ParamValueContainer &dst) const override;

		StringParam(const StringParam &o) = delete;
		StringParam(StringParam &&o) = delete;
		StringParam(const StringParam &&o) = delete;

		StringParam &operator=(const StringParam &other) = delete;
		StringParam &operator=(StringParam &&other) = delete;
		StringParam &operator=(const StringParam &&other) = delete;

	private:
		std::string value_;
		std::string default_;
	};

	class DoubleParam: public Param {
	public:
		DoubleParam(double value, const char* name, const char* comment, ParamsVector& owner, bool init = false, ParamOnModifyFunction on_modify_f = 0);
		virtual ~DoubleParam() = default;

		operator double() const;
		void operator=(double value);

		virtual bool set_value(const char *v, SOURCE_REF) override;
		virtual bool set_value(int32_t v, SOURCE_REF) override;
		virtual bool set_value(bool v, SOURCE_REF) override;
		virtual bool set_value(double v, SOURCE_REF) override;

		double value() const;

		virtual void ResetToDefault(SOURCE_TYPE) override;
		virtual void ResetFrom(const ParamsVectorSet& vec, SOURCE_TYPE) override;

		virtual std::string formatted_value_str() const override;
		virtual std::string raw_value_str() const override;
		virtual const char* value_type_str() const override;

		virtual bool inspect_value(ParamValueContainer &dst) const override;

		DoubleParam(const DoubleParam &o) = delete;
		DoubleParam(DoubleParam &&o) = delete;
		DoubleParam(const DoubleParam &&o) = delete;

		DoubleParam &operator=(const DoubleParam &other) = delete;
		DoubleParam &operator=(DoubleParam &&other) = delete;
		DoubleParam &operator=(const DoubleParam &&other) = delete;

	private:
		double value_;
		double default_;
	};


	// remove the macros to help set up the member prototypes

#undef SOURCE_TYPE															
#undef SOURCE_REF																



// Global parameter lists.
//
// To avoid the problem of undetermined order of static initialization
// global_params are accessed through the GlobalParams function that
// initializes the static pointer to global_params only on the first time
// GlobalParams() is called.
//
// TODO(daria): remove GlobalParams() when all global Tesseract
// parameters are converted to members.
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

	 // ------------------------------------

	extern BOOL_VAR_H(stream_filelist);
	extern STRING_VAR_H(document_title);
#ifdef HAVE_LIBCURL
	extern INT_VAR_H(curl_timeout);
	extern STRING_VAR_H(curl_cookiefile);
#endif
	extern INT_VAR_H(debug_all);
	extern BOOL_VAR_H(debug_misc);
	extern BOOL_VAR_H(verbose_process);
#if !GRAPHICS_DISABLED
	extern BOOL_VAR_H(scrollview_support);
#endif
	extern STRING_VAR_H(vars_report_file);
	extern BOOL_VAR_H(report_all_variables);
	extern DOUBLE_VAR_H(allowed_image_memory_capacity);
	extern BOOL_VAR_H(two_pass);

} // namespace tesseract

#endif
