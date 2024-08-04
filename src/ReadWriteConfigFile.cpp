
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {




	// Utility functions for working with Tesseract parameters.
	class ExtraUtils {
	public:
		// Read parameters from the given file pointer.
		// Otherwise identical to ReadParamsFile().
		static bool ReadParamsFromFp(FILE *fp,
																 const ParamsVectorSet &set,
																 ParamSetBySourceType source_type = PARAM_VALUE_IS_SET_BY_APPLICATION,
																 ParamPtr source = nullptr);

	};






}	// namespace
