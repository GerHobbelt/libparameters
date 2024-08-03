
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {






	template<>
	IntParam *FindParam(const char *name, ParamsVectorSet *globals, ParamsVectorSet *locals, const IntParam *DUMMY, ParamType accepted_types_mask) {
		if (!globals)
			globals = ::tesseract::GlobalParams();

		Param *p = globals->find(name);
		if (!p && locals != nullptr) {
			p = locals->find(name);
		}
		IntParam *rv = nullptr;
		if (p && p->type() == INT_PARAM)
			rv = static_cast<IntParam *>(p);

		if (rv)
			return rv;

		return nullptr;
	}


	bool ParamUtils::SetParam(const char *name, const char *value,
														ParamsVectorSet *member_params) {
		// Look for the parameter among string parameters.
		auto *sp = FindParam<StringParam>(name, GlobalParams(), member_params);
		if (sp != nullptr) {
			sp->set_value(value);
		}
		if (*value == '\0') {
			return (sp != nullptr);
		}

		// Look for the parameter among int parameters.
		auto *ip = FindParam<IntParam>(name, GlobalParams(), member_params);
		if (ip) {
			int intval = INT_MIN;
			std::stringstream stream(value);
			stream.imbue(std::locale::classic());
			stream >> intval;
			if (intval == 0) {
				std::string sv(stream.str());
				if (!sv.empty() &&
						(sv[0] == 'T' || sv[0] == 't' || sv[0] == 'Y' || sv[0] == 'y')) {
					intval = 1;
				}
			}
			if (intval != INT_MIN) {
				ip->set_value(intval);
			}
		}

		// Look for the parameter among bool parameters.
		auto *bp = FindParam<BoolParam>(name, GlobalParams(), member_params);
		if (bp != nullptr && bp->constraint_ok(constraint)) {
			if (*value == 'T' || *value == 't' || *value == 'Y' || *value == 'y' || *value == '1') {
				bp->set_value(true);
			} else if (*value == 'F' || *value == 'f' || *value == 'N' || *value == 'n' || *value == '0') {
				bp->set_value(false);
			}
		}

		// Look for the parameter among double parameters.
		auto *dp = FindParam<DoubleParam>(name, GlobalParams(), member_params);
		if (dp != nullptr) {
			double doubleval = NAN;
			std::stringstream stream(value);
			stream.imbue(std::locale::classic());
			stream >> doubleval;
			if (!std::isnan(doubleval)) {
				dp->set_value(doubleval);
			}
		}
		return (sp || ip || bp || dp);
	}

	bool ParamUtils::GetParamAsString(const char *name, const ParamsVectorSet *member_params,
																		std::string *value) {
		// Look for the parameter among string parameters.
		auto *sp = FindParam<StringParam>(name, GlobalParams(), member_params);
		if (sp) {
			*value = sp->c_str();
			return true;
		}
		// Look for the parameter among int parameters.
		auto *ip = FindParam<IntParam>(name, GlobalParams(), member_params);
		if (ip) {
			*value = std::to_string(int32_t(*ip));
			return true;
		}
		// Look for the parameter among bool parameters.
		auto *bp = FindParam<BoolParam>(name, GlobalParams(), member_params);
		if (bp != nullptr) {
			*value = bool(*bp) ? "1" : "0";
			return true;
		}
		// Look for the parameter among double parameters.
		auto *dp = FindParam<DoubleParam>(name, GlobalParams(), member_params);
		if (dp != nullptr) {
			std::ostringstream stream;
			stream.imbue(std::locale::classic());
			stream << double(*dp);
			*value = stream.str();
			return true;
		}
		return false;
	}



}	// namespace
