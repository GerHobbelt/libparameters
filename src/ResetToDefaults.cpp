
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {





	void ParamUtils::ResetToDefaults(const ParamsVectorSet& set, ParamSetBySourceType source_type) {
#if 0
		for (Param *param : GlobalParams().as_list()) {
			param->ResetToDefault(set, source_type);
		}
#endif
		for (Param *param : set.as_list()) {
			param->ResetToDefault(set, source_type);
		}
	}



}	// namespace
