
#include <parameters/parameters.h>

#include "internal_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamsVectorSet
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	ParamsVectorSet::~ParamsVectorSet() {
		collection_.clear();
	}

	ParamsVectorSet::ParamsVectorSet() {
	}

	ParamsVectorSet::ParamsVectorSet(std::initializer_list<ParamsVector *> vecs) {
		for (ParamsVector *i : vecs) {
			add(i);
		}
	}

	void ParamsVectorSet::add(ParamsVector &vec_ref) {
		collection_.push_back(&vec_ref);
	}

	void ParamsVectorSet::add(ParamsVector *vec_ref) {
		collection_.push_back(vec_ref);
	}

	void ParamsVectorSet::add(std::initializer_list<ParamsVector *> vecs) {
		for (ParamsVector *i : vecs) {
			add(i);
		}
	}

	void ParamsVectorSet::add(const ParamsVectorSet &vecset_ref) {
		for (ParamsVector *i : vecset_ref.collection_) {
			add(i);
		}
	}
	void ParamsVectorSet::add(const ParamsVectorSet *vecset_ref) {
		if (vecset_ref == nullptr)
			return;
		for (ParamsVector *i : vecset_ref->collection_) {
			add(i);
		}
	}

	const std::vector<ParamsVector *> &ParamsVectorSet::get() const {
		return collection_;
	}

	Param *ParamsVectorSet::find(
		const char *name,
		ParamType accepted_types_mask
	) const {
		for (ParamsVector *vec : collection_) {
			auto l = vec->params_.find(name);
			if (l != vec->params_.end()) {
				ParamPtr p = (*l).second;
				if ((p->type() & accepted_types_mask) != 0) {
					return p;
				}
			}
		}
		return nullptr;
	}

	template <>
	IntParam *ParamsVectorSet::find<IntParam>(
		const char *name
	) const {
		return static_cast<IntParam *>(find(name, INT_PARAM));
	}

	template <>
	BoolParam *ParamsVectorSet::find<BoolParam>(
		const char *name
	) const {
		return static_cast<BoolParam *>(find(name, BOOL_PARAM));
	}

	template <>
	DoubleParam *ParamsVectorSet::find<DoubleParam>(
		const char *name
	) const {
		return static_cast<DoubleParam *>(find(name, DOUBLE_PARAM));
	}

	template <>
	StringParam *ParamsVectorSet::find<StringParam>(
		const char *name
	) const {
		return static_cast<StringParam *>(find(name, STRING_PARAM));
	}

	template <>
	Param* ParamsVectorSet::find<Param>(
		const char* name
	) const {
		return static_cast<Param*>(find(name, ANY_TYPE_PARAM));
	}

	std::vector<ParamPtr> ParamsVectorSet::as_list(
		ParamType accepted_types_mask
	) const {
		std::vector<ParamPtr> lst;
		for (ParamsVector *vec : collection_) {
			for (auto i : vec->params_) {
				ParamPtr p = i.second;
				if ((p->type() & accepted_types_mask) != 0) {
					lst.push_back(p);
				}
			}
		}
		return lst;
	}

	ParamsVector ParamsVectorSet::flattened_copy(
		ParamType accepted_types_mask
	) const {
		ParamsVector rv("muster");

		std::vector<ParamPtr> srclst = as_list(accepted_types_mask);
		for (ParamPtr ref : srclst) {
			rv.add(ref);
		}
		return rv;
	}

} // namespace tesseract
