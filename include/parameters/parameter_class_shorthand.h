/*
 * Class definitions of the *_VAR classes for tunable constants.
 *
 * UTF8 detect helper statement: «bloody MSVC»
*/

#ifndef _LIB_PARAMS_CLASSES_SHORTHAND_H_
#define _LIB_PARAMS_CLASSES_SHORTHAND_H_

#include <parameters/parameter_class_fundamentals.h>
#include <parameters/parameter_class_value_typed.h>
#include <parameters/parameter_class_string.h>
#include <parameters/parameter_class_ref_typed.h>
#include <parameters/parameter_class_userdef.h>
#include <parameters/parameter_class_vector_value_typed.h>
#include <parameters/parameter_class_vector_ref_typed.h>
#include <parameters/fmt-support.h>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>


namespace parameters {

	// see note above: these must be using statements, not derived classes, or otherwise the constructor/operator delete instructions in that base template won't deliver as expected!

	using IntParam = ValueTypedParam<int32_t, BasicValueParamParseAssistant>;
	using BoolParam = ValueTypedParam<bool, BasicValueParamParseAssistant>;
	using DoubleParam = ValueTypedParam<double, BasicValueParamParseAssistant>;

	using StringParam = StringTypedParam<std::string, BasicStringParamParseAssistant>;

	// --------------------------------------------------------------------------------------------------

	using StringSetParam = BasicVectorTypedParam<std::string, BasicVectorParamParseAssistant>;
	using IntSetParam = BasicVectorTypedParam<int32_t, BasicVectorParamParseAssistant>;
	using BoolSetParam = BasicVectorTypedParam<bool, BasicVectorParamParseAssistant>;
	using DoubleSetParam = BasicVectorTypedParam<double, BasicVectorParamParseAssistant>;

} // namespace 

#endif
