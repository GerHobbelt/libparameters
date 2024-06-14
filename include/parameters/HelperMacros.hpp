/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_MACROS_H_
#define _LIB_PARAMS_MACROS_H_

#include <parameters/parameters.h>
#include <parameters/parameter_globals.h>


	/*************************************************************************
	 * Note on defining parameters.
	 *
	 * The values of the parameters defined with *_INIT_* macros are guaranteed
	 * to be loaded from config files before Tesseract initialization is done
	 * (there is no such guarantee for parameters defined with the other macros).
	 *************************************************************************/

#define INT_VAR_H(name) ::parameters::IntParam name

#define BOOL_VAR_H(name) ::parameters::BoolParam name

#define STRING_VAR_H(name) ::parameters::StringParam name

#define DOUBLE_VAR_H(name) ::parameters::DoubleParam name

#define INT_VAR(name, val, comment) \
  ::parameters::IntParam name(val, #name, comment, ::parameters::GlobalParams())

#define BOOL_VAR(name, val, comment) \
  ::parameters::BoolParam name(val, #name, comment, ::parameters::GlobalParams())

#define STRING_VAR(name, val, comment) \
  ::parameters::StringParam name(val, #name, comment, ::parameters::GlobalParams())

#define DOUBLE_VAR(name, val, comment) \
  ::parameters::DoubleParam name(val, #name, comment, ::parameters::GlobalParams())

#define INT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define BOOL_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define STRING_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define DOUBLE_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define INT_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define BOOL_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define STRING_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#define DOUBLE_INIT_MEMBER(name, val, comment, vec) name(val, #name, comment, vec)

#endif
