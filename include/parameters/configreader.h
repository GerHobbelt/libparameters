/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_CONFIGREADER_H_
#define _LIB_PARAMS_CONFIGREADER_H_

#include <cstdint>
#include <string>


namespace parameters {

	// --------------------------------------------------------------------------------------------------

	// ConfigReader, et al, used as support classes for the paramaeter usage APIs ReadParamsFile() et al.

	// The idea of the ConfigReader class hierarchy is this:
	// 
	// We can read/fetch from one or more targets, each of arbitrary type, e.g. stdin (UNIX stdio pipe), config file, byte array / string,
	// using any arbitrary format which is easily translated to what libparameters expects for input:
	//
	// `#`, `//` and `;` start comment lines, while parameter value lines come out of the reader formatted as
	// `param_name    param_value`, i.e. no '=' assignment operator (as that one is *implicit*) and arbitrary
	// whitespace separating name and value.
	//
	// To help process arbitrary string values, we accept quoted string values and process them in the ReadParamsFile() API function.
	// 
	// Note that this specification is intended to be able for us to 'round trip', i.e. be able to help us parse any 'config file'
	// produced by our own WriteParamsFile() API function.

	class ConfigReader {
	public:
		ConfigReader() {}
		virtual ~ConfigReader() = default;

		struct line {
			std::string_view content;
			unsigned int linenumber;
			bool EOF_reached;
			bool error;

			line() {
				init();
			}
			~line() = default;
			void init() {
				content = std::string_view(); // .clear();
				linenumber = 0;
				EOF_reached = false;
				error = false;
			}
		};

		// Return false when there's an error or EOF reached, i.e. returns true
		// as long as there's potentially more valid content to be read.
		virtual bool ReadInfoLine(line &line) = 0;

	protected:
		unsigned int _lineno{0};
	};

} // namespace 

#endif
