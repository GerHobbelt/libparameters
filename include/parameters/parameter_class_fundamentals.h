/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_CLASSES_FUNDAMENTALS_H_
#define _LIB_PARAMS_CLASSES_FUNDAMENTALS_H_

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
		CUSTOM_PARAM =      0x0010, // a yet-unspecified type: provided as an advanced-use generic parameter value storage container for when the other, basic, value types do not suffice in userland code. The tesseract core does not employ this value type anywhere: we do have compound paramater values, mostly sets of file paths, but those are encoded as *string* value in their parameter value.

		VECTOR_PARAM =      0x0020, // any vector type.

		INT_SET_PARAM =     INT_PARAM | VECTOR_PARAM,
		BOOL_SET_PARAM =    BOOL_PARAM | VECTOR_PARAM,
		DOUBLE_SET_PARAM =  DOUBLE_PARAM | VECTOR_PARAM,
		STRING_SET_PARAM =  STRING_PARAM | VECTOR_PARAM,
		CUSTOM_SET_PARAM =  CUSTOM_PARAM | VECTOR_PARAM, // a yet-unspecified vector type.

		ANY_TYPE_PARAM =    0x03F, // catch-all identifier for the selection/filter functions: there this is used to match *any and all* parameter value types encountered.
	};
	DECL_FMT_FORMAT_PARAMENUMTYPE(ParamType);

	static inline auto format_as(ParamType t) {
		return fmt::underlying(t);
	}

	// Identifiers used to indicate the *origin* of the current parameter value. Used for reporting/diagnostic purposes. Do not treat these
	// as gospel; these are often assigned under limited/reduced information conditions, so they merely serve as report *hints*.
	enum ParamSetBySourceType : unsigned {
		PARAM_VALUE_IS_DEFAULT = 0,

		PARAM_VALUE_IS_RESET,
		PARAM_VALUE_IS_SET_BY_PRESET,          // 'indirect' write: a tesseract 'preset' parameter set was invoked and that one set this one as part of the action.
		PARAM_VALUE_IS_SET_BY_CONFIGFILE,      // 'explicit' write by loading and processing a config file.
		PARAM_VALUE_IS_SET_BY_ASSIGN,		   // 'indirect' write: value is copied over from elsewhere via operator=.
		PARAM_VALUE_IS_SET_BY_PARAM,           // 'indirect' write: other Param's OnChange code set the param value, whatever it is now.
		PARAM_VALUE_IS_SET_BY_APPLICATION,     // 'explicit' write: user / application code set the param value, whatever it is now.
		PARAM_VALUE_IS_SET_BY_CORE_RUN,        // 'explicit' write by the running application core: before proceding with the next step the run-time adjusts this one, e.g. (incrementing) page number while processing a multi-page OCR run.
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
	concept ParamAcceptableOtherType = !ParamAcceptableValueType<T>;

	static_assert(ParamAcceptableValueType<int>);
	static_assert(ParamAcceptableValueType<double>);
	static_assert(ParamAcceptableValueType<bool>);
	static_assert(!ParamAcceptableValueType<const char *>);
	static_assert(!ParamAcceptableValueType<char*>);
	static_assert(!ParamAcceptableValueType<const wchar_t *>);
	static_assert(!ParamAcceptableValueType<wchar_t*>);
	static_assert(!ParamAcceptableValueType<std::string>);
	static_assert(!ParamAcceptableValueType<std::string&>);
	static_assert(!ParamAcceptableValueType<std::wstring>);
	static_assert(!ParamAcceptableValueType<std::wstring&>);
	static_assert(ParamAcceptableOtherType<ParamVoidPtrDataType>);
	static_assert(ParamAcceptableOtherType<ParamArbitraryOtherType>);
	static_assert(ParamAcceptableOtherType<ParamStringSetType>);
	static_assert(ParamAcceptableOtherType<ParamIntSetType>);
	static_assert(ParamAcceptableOtherType<ParamBoolSetType>);
	static_assert(ParamAcceptableOtherType<ParamDoubleSetType>);

	template<typename T>
	concept ParamDerivativeType = std::is_base_of<Param, T>::value;

#else

#define ParamAcceptableValueType   class

#define ParamDerivativeType   typename

#endif

	// --- END of section setting up various C++ template constraint helpers ---


	// --------------------------------------------------------------------------------------------------

	// A set (vector) of parameters. While this is named *vector* the internal organization
	// is hash table based to provide fast random-access add / remove / find functionality.
	class ParamsVector;

	// --------------------------------------------------------------------------------------------------

	// an (ad-hoc?) collection of ParamsVector instances.
	class ParamsVectorSet;

} // namespace 

#endif
