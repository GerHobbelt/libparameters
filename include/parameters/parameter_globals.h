/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_GLOBALS_H_
#define _LIB_PARAMS_GLOBALS_H_

namespace parameters {

	class ParamsVector;

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

} // namespace 

#endif
