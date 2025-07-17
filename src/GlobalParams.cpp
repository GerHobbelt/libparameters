
#include <parameters/parameters.h>

#include "internal_helpers.hpp"


namespace parameters {

	ParamsVector &GlobalParams(void) {
		static ParamsVector global_params("global"); // static auto-inits at startup
		return global_params;
	}

} // namespace tesseract
