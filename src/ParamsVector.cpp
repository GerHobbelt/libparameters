
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


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamsVector
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	ParamsVector::~ParamsVector() {
		if (is_params_owner_) {
			// we are the owner of all these Param instances, so we should destroy them here!
			for (auto i : params_) {
				ParamPtr p = i.second;
				delete p;
			}
		}
		params_.clear();
	}

	ParamsVector::ParamsVector(const char *title):
		title_(title)
	{
		params_.reserve(256);
	}

	// Note: std::initializer_list<ParamRef> causes compiler errors: error C2528: 'abstract declarator': you cannot create a pointer to a reference 
	// See also: https://skycoders.wordpress.com/2014/06/08/in-c-pointer-to-reference-is-illegal/
	// hence this is a bug:
	//   ParamsVector::ParamsVector(const char *title, std::initializer_list<ParamRef> vecs) : ......

	ParamsVector::ParamsVector(const char *title, std::initializer_list<ParamPtr> vecs):
		title_(title)
	{
		params_.reserve(256);

		for (ParamPtr i : vecs) {
			add(i);
		}
	}

	void ParamsVector::mark_as_all_params_owner() {
		is_params_owner_ = true;
	}

	void ParamsVector::add(ParamPtr param_ref) {
		check_and_report_name_collisions(param_ref->name_str(), params_);
		params_.insert({param_ref->name_str(), param_ref});
	}

	void ParamsVector::add(Param &param_ref) {
		add(&param_ref);
	}

	void ParamsVector::add(std::initializer_list<ParamPtr> vecs) {
		for (ParamPtr i : vecs) {
			add(i);
		}
	}

	void ParamsVector::remove(const char *name) {
		if (!name || !*name)
			return;
		params_.erase(name);
	}

	void ParamsVector::remove(ParamPtr param_ref) {
		remove(param_ref->name_str());
	}

	void ParamsVector::remove(ParamRef param_ref) {
		remove(&param_ref);
	}


	Param *ParamsVector::find(
		const char *name,
		ParamType accepted_types_mask
	) const {
		auto l = params_.find(name);
		if (l != params_.end()) {
			ParamPtr p = (*l).second;
			if ((p->type() & accepted_types_mask) != 0) {
				return p;
			}
		}
		return nullptr;
	}


	template <>
	IntParam *ParamsVector::find<IntParam>(
		const char *name
	) const {
		return static_cast<IntParam *>(find(name, INT_PARAM));
	}

	template <>
	BoolParam *ParamsVector::find<BoolParam>(
		const char *name
	) const {
		return static_cast<BoolParam *>(find(name, BOOL_PARAM));
	}

	template <>
	DoubleParam *ParamsVector::find<DoubleParam>(
		const char *name
	) const {
		return static_cast<DoubleParam *>(find(name, DOUBLE_PARAM));
	}

	template <>
	StringParam *ParamsVector::find<StringParam>(
		const char *name
	) const {
		return static_cast<StringParam *>(find(name, STRING_PARAM));
	}

	template <>
	Param* ParamsVector::find<Param>(
		const char* name
	) const {
		return find(name, ANY_TYPE_PARAM);
	}


	std::vector<ParamPtr> ParamsVector::as_list(
		ParamType accepted_types_mask
	) const {
		std::vector<ParamPtr> lst;
		for (auto i : params_) {
			ParamPtr p = i.second;
			if ((p->type() & accepted_types_mask) != 0) {
				lst.push_back(p);
			}
		}
		return lst;
	}

	const char* ParamsVector::title() const {
		return title_.c_str();
	}
	void ParamsVector::change_title(const char* title) {
		title_ = title ? title : "";
	}



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

#define THE_4_HANDLERS_PROTO_4_SURPLUS(type)																								\
  const char *name, const char *comment, bool init,						                      				\
	type::ParamOnModifyFunction on_modify_f, type::ParamOnValidateFunction on_validate_f,			\
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
