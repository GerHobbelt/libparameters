
// Readability helper: reduce noise by using macros to help set up the member prototypes.

#define SOURCE_TYPE																                        \
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION

#define SOURCE_REF																                        \
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION,	\
		ParamPtr source = nullptr
