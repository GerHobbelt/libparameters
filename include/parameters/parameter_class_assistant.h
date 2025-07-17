/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_CLASSES_ASSISTANT_H_
#define _LIB_PARAMS_CLASSES_ASSISTANT_H_

#include <string>


namespace parameters {

	struct BasicVectorParamParseAssistant {
		std::string parse_separators{"\t\r\n,;:|"}; //< list of separators accepted by the string parse handler. Any one of these separates individual elements in the array.

		// For formatting the set for data serialization / save purposes, the generated set may be wrapped in a prefix and postfix, e.g. "{" and "}".
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

	// --------------------------------------------------------------------------------------------------

	struct BasicValueParamParseAssistant {
		// For formatting the value for data serialization / save purposes, the generated value may be wrapped in a prefix and postfix, e.g. "{" and "}".
		std::string fmt_data_prefix{""};
		std::string fmt_data_postfix{""};

		// For formatting the value for display purposes, the generated value may be wrapped in a prefix and postfix, e.g. "[" and "]".
		std::string fmt_display_prefix{""};
		std::string fmt_display_postfix{""};

		bool parse_should_cope_with_fmt_display_prefixes{true}; //< when true, the registered string parse handler is supposed to be able to cope with encountering the format display-output prefix and prefix strings.
		bool parse_trims_surrounding_whitespace{true};  //< the string parse handler will trim any whitespace occurring before or after every value stored in the string.
	};

	// --------------------------------------------------------------------------------------------------

	struct BasicStringParamParseAssistant {
		// For formatting the value for data serialization / save purposes, the generated value may be wrapped in a prefix and postfix, e.g. "{" and "}".
		std::string fmt_data_prefix{"\""};
		std::string fmt_data_postfix{"\""};

		// For formatting the value for display purposes, the generated value may be wrapped in a prefix and postfix, e.g. "[" and "]".
		std::string fmt_display_prefix{"\""};
		std::string fmt_display_postfix{"\""};

		bool parse_should_cope_with_fmt_display_prefixes{true}; //< when true, the registered string parse handler is supposed to be able to cope with encountering the format display-output prefix and prefix strings.
		bool parse_trims_surrounding_whitespace{true};  //< the string parse handler will trim any whitespace occurring before or after every value stored in the string.
	};

} // namespace 

#endif
