
#include <parameters/parameters.h>

#include "internal_helpers.hpp"


namespace parameters {

	ParamsVector &GlobalParams() {
		static ParamsVector global_params("global"); // static auto-inits at startup
		return global_params;
	}


	static ParamSetBySourceType default_source_type = PARAM_VALUE_IS_SET_BY_ASSIGN;

	/**
	 * The default application source_type starts out as PARAM_VALUE_IS_SET_BY_ASSIGN.
	 * Discerning applications may want to set the default source type to PARAM_VALUE_IS_SET_BY_APPLICATION
	 * or PARAM_VALUE_IS_SET_BY_CONFIGFILE, depending on where the main workflow is currently at,
	 * while the major OCR tesseract APIs will set source type to PARAM_VALUE_IS_SET_BY_CORE_RUN
	 * (if the larger, embedding, application hasn't already).
	 *
	 * The purpose here is to be able to provide improved diagnostics reports about *who* did *what* to
	 * *which* parameters *when* exactly.
	 */
	void ParamUtils::set_current_application_default_param_source_type(ParamSetBySourceType source_type) {
		default_source_type = source_type;
	}

	/**
	 * Produces the current default application source type; intended to be used internally by our parameters support library code.
	 */
	ParamSetBySourceType ParamUtils::get_current_application_default_param_source_type() {
		return default_source_type;
	}

} // namespace tesseract
