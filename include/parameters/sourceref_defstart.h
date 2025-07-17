
// Readability helper: reduce noise by using macros to help set up the member prototypes.

#define SOURCE_TYPE																                        \
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION

#define SOURCE_REF																                        \
		ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION,							\
		ParamPtr source = nullptr

#define THE_4_HANDLERS_PROTO																			\
      ParamOnModifyFunction on_modify_f = 0, ParamOnValidateFunction on_validate_f = 0,					\
      ParamOnParseFunction on_parse_f = 0, ParamOnFormatFunction on_format_f = 0

#define THE_4_HANDLERS_PROTO_4_SURPLUS(type)															\
      const char *name, const char *comment, bool init = false,		                      				\
	  type::ParamOnModifyFunction on_modify_f = 0, type::ParamOnValidateFunction on_validate_f = 0,		\
	  type::ParamOnParseFunction on_parse_f = 0, type::ParamOnFormatFunction on_format_f = 0



// --------------------------------------------------------------------------------------------------



#define THE_4_HANDLERS_PROTO_4_IMPL																		\
      ParamOnModifyFunction on_modify_f, ParamOnValidateFunction on_validate_f,							\
      ParamOnParseFunction on_parse_f, ParamOnFormatFunction on_format_f

#define THE_4_HANDLERS_PROTO_4_SURPLUS_IMPL(type)														\
      const char *name, const char *comment, bool init,		                      						\
	  type::ParamOnModifyFunction on_modify_f, type::ParamOnValidateFunction on_validate_f,				\
	  type::ParamOnParseFunction on_parse_f, type::ParamOnFormatFunction on_format_f



// --------------------------------------------------------------------------------------------------


#if 0
ValueTypedParam(const char *value, const Assistant &assist, TheEventHandlers userdef_handlers);
ValueTypedParam(const T value, const Assistant &assist, THE_4_HANDLERS_PROTO);
explicit ValueTypedParam(const T *value, const Assistant &assist, THE_4_HANDLERS_PROTO);

MK_CONSTRUCTORS(ValueTypedParam, const char *value);
MK_CONSTRUCTORS(ValueTypedParam, const T value);
MK_EXPLICIT_CONSTRUCTORS(ValueTypedParam, const T *value);

const char *name, const char *comment, ParamsVector &owner, bool init = false, 

const char *name, const char *comment, ParamsVector &owner, bool init, 

#endif

#define MK_CONSTRUCTORS(XYZTypedParam, value_arg_decl)																												\
XYZTypedParam(value_arg_decl, const char *name, const char *comment, ParamsVector &owner, bool init, TheEventHandlers userdef_handlers = {});						\
XYZTypedParam(value_arg_decl, const char *name, const char *comment, ParamsVector &owner, TheEventHandlers userdef_handlers = {});									\
XYZTypedParam(value_arg_decl, const char *name, const char *comment, bool init, TheEventHandlers userdef_handlers = {});											\
XYZTypedParam(value_arg_decl, const char *name, const char *comment, TheEventHandlers userdef_handlers = {})

#define MK_EXPLICIT_CONSTRUCTORS(XYZTypedParam, value_arg_decl)																										\
explicit XYZTypedParam(value_arg_decl, const char *name, const char *comment, ParamsVector &owner, bool init, TheEventHandlers userdef_handlers = {});				\
explicit XYZTypedParam(value_arg_decl, const char *name, const char *comment, ParamsVector &owner, TheEventHandlers userdef_handlers = {});							\
explicit XYZTypedParam(value_arg_decl, const char *name, const char *comment, bool init, TheEventHandlers userdef_handlers = {});									\
explicit XYZTypedParam(value_arg_decl, const char *name, const char *comment, TheEventHandlers userdef_handlers = {})

