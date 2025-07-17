
#include <parameters/parameters.h>

#include "internal_helpers.hpp"


namespace parameters {

#ifndef NDEBUG
	static inline void check_and_report_name_collisions(const char *name, const ParamsHashTableType &table) {
		if (table.contains(name)) {
			std::string s = fmt::format("{} param name '{}' collision: double definition of param '{}'", ParamUtils::GetApplicationName(), name, name);
			throw new std::logic_error(s);
		}
	}
	static inline void check_and_report_name_collisions(const char *name, std::vector<ParamPtr> &table) {
		for (Param *p : table) {
			if (ParamHash()(p->name_str(), name)) {
				std::string s = fmt::format("{} param name '{}' collision: double definition of param '{}'", ParamUtils::GetApplicationName(), name, name);
				throw new std::logic_error(s);
			}
		}
	}
#else
#define check_and_report_name_collisions(name, table)     ((void)0)
#endif




	//////////////////////////////////////////////////////////////////////////////////////////////
	//
	// SurplusParamVector
	//
	//////////////////////////////////////////////////////////////////////////////////////////////


// A set (vector) of surplus parameters, i.e. parameters which are defined at run-time, rather than at compile-time.
// This SurplusParamsVector class is the owner of each of these (heap allocated) parameters, which are created on demand
// when calling the add() method.

	SurplusParamsVector::SurplusParamsVector(const char* title)
		: ParamsVector(title)
	{
		is_params_owner_ = true;
	}

#define THE_4_HANDLERS_PROTO_4_SURPLUS(type)																\
  const char *name, const char *comment, bool init,						                      				\
	type::ParamOnModifyFunction on_modify_f, type::ParamOnValidateFunction on_validate_f,					\
			type::ParamOnParseFunction on_parse_f, type::ParamOnFormatFunction on_format_f


	SurplusParamsVector::~SurplusParamsVector() {}

	void SurplusParamsVector::add(const int32_t value, THE_4_HANDLERS_PROTO_4_SURPLUS(IntParam)) {
		IntParam *param = new IntParam(value, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
		add(param);
	}
		void SurplusParamsVector::add(const bool value, THE_4_HANDLERS_PROTO_4_SURPLUS(BoolParam)) {
			BoolParam *param = new BoolParam(value, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}
		void SurplusParamsVector::add(const double value, THE_4_HANDLERS_PROTO_4_SURPLUS(DoubleParam)) {
			DoubleParam *param = new DoubleParam(value, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}
		void SurplusParamsVector::add(const std::string &value, THE_4_HANDLERS_PROTO_4_SURPLUS(StringParam)) {
			StringParam *param = new StringParam(value, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}
		void SurplusParamsVector::add(const std::vector<int32_t> value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(IntSetParam)) {
			IntSetParam *param = new IntSetParam(value, assistant, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}
		void SurplusParamsVector::add(const std::vector<bool> value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(BoolSetParam)) {
			BoolSetParam *param = new BoolSetParam(value, assistant, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}
		void SurplusParamsVector::add(const std::vector<double> value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(DoubleSetParam)) {
			DoubleSetParam *param = new DoubleSetParam(value, assistant, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}
		void SurplusParamsVector::add(const std::vector<std::string> &value, const BasicVectorParamParseAssistant &assistant, THE_4_HANDLERS_PROTO_4_SURPLUS(StringSetParam)) {
			StringSetParam *param = new StringSetParam(value, assistant, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}

		void SurplusParamsVector::add(const char *value, THE_4_HANDLERS_PROTO_4_SURPLUS(StringParam)) {
			StringParam *param = new StringParam(value, name, comment, *this, init, on_modify_f, on_validate_f, on_parse_f, on_format_f);
			add(param);
		}

		void SurplusParamsVector::add(ParamPtr param_ref) {
			ParamsVector::add(param_ref);
		}
		void SurplusParamsVector::add(std::initializer_list<ParamPtr> vecs) {
			ParamsVector::add(vecs);
		}

} // namespace tesseract
