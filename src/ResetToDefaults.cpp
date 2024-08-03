
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {





	void ParamUtils::ResetToDefaults(const ParamsVectorSet& set, ParamSetBySourceType source_type) {

	}





	// Resets all parameters back to default values;
	void ParamUtils::ResetToDefaults(ParamsVectorSet *member_params) {
		for (Param *param : GlobalParams()->as_list()) {
			param->ResetToDefault();
		}
		if (member_params != nullptr) {
			for (Param *param : member_params->as_list()) {
				param->ResetToDefault();
			}
		}
	}



}	// namespace
