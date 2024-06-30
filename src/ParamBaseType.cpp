
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


namespace parameters {

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Param
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	Param::Param(const char *name, const char *comment, ParamsVector &owner, bool init)
		: owner_(owner),
		name_(name),
		info_(comment),
		init_(init),
		// debug_(false),
		set_(false),
		set_to_non_default_value_(false),
		locked_(false),
		error_(false),

		type_(UNKNOWN_PARAM),
		set_mode_(PARAM_VALUE_IS_DEFAULT),
		setter_(nullptr),
		access_counts_({0, 0, 0, 0})
	{
		debug_ = (strstr(name, "debug") != nullptr) || (strstr(name, "display") != nullptr);

		owner.add(this);
	}

	const char *Param::name_str() const noexcept {
		return name_;
	}
	const char *Param::info_str() const noexcept {
		return info_;
	}
	bool Param::is_init() const noexcept {
		return init_;
	}
	bool Param::is_debug() const noexcept {
		return debug_;
	}
	bool Param::is_set() const noexcept {
		return set_;
	}
	bool Param::is_set_to_non_default_value() const noexcept {
		return set_to_non_default_value_;
	}
	bool Param::is_locked() const noexcept {
		return locked_;
	}
	bool Param::has_faulted() const noexcept {
		return error_;
	}

	void Param::lock(bool locking) {
		locked_ = locking;
	}
	void Param::fault() noexcept {
		safe_inc(access_counts_.faulting);
		error_ = true;
	}

	void Param::reset_fault() noexcept {
		error_ = false;
	}

	ParamSetBySourceType Param::set_mode() const noexcept {
		return set_mode_;
	}
	Param *Param::is_set_by() const noexcept {
		return setter_;
	}

	ParamsVector &Param::owner() const noexcept {
		return owner_;
	}

	const Param::access_counts_t &Param::access_counts() const noexcept {
		return access_counts_;
	}

	void Param::reset_access_counts() noexcept {
		safe_add(access_counts_.prev_sum_reading, access_counts_.reading);
		safe_add(access_counts_.prev_sum_writing, access_counts_.writing);
		safe_add(access_counts_.prev_sum_changing, access_counts_.changing);

		access_counts_.reading = 0;
		access_counts_.writing = 0;
		access_counts_.changing = 0;
	}

	std::string Param::formatted_value_str() const {
		return value_str(VALSTR_PURPOSE_DATA_FORMATTED_4_DISPLAY);
	}

	std::string Param::raw_value_str() const {
		return value_str(VALSTR_PURPOSE_RAW_DATA_4_INSPECT);
	}

	std::string Param::formatted_default_value_str() const {
		return value_str(VALSTR_PURPOSE_DEFAULT_DATA_FORMATTED_4_DISPLAY);
	}

	std::string Param::raw_default_value_str() const {
		return value_str(VALSTR_PURPOSE_RAW_DEFAULT_DATA_4_INSPECT);
	}

	std::string Param::raw_value_type_str() const {
		return value_str(VALSTR_PURPOSE_TYPE_INFO_4_INSPECT);
	}

	std::string Param::value_type_str() const {
		return value_str(VALSTR_PURPOSE_TYPE_INFO_4_DISPLAY);
	}

	void Param::set_value(const std::string &v, ParamSetBySourceType source_type, ParamPtr source) {
		set_value(v.c_str(), source_type, source);
	}

	void Param::operator=(const char *value) {
		set_value(value, ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}
	void Param::operator=(const std::string &value) {
		set_value(value.c_str(), ParamUtils::get_current_application_default_param_source_type(), nullptr);
	}

	void Param::ResetToDefault(const ParamsVectorSet &source_vec, ParamSetBySourceType source_type) {
		ResetToDefault(&source_vec, source_type);
	}

	ParamType Param::type() const noexcept {
		return type_;
	}

#if 0
	bool Param::set_value(const ParamValueContainer &v, ParamSetBySourceType source_type, ParamPtr source) {
		if (const int32_t* val = std::get_if<int32_t>(&v))
			return set_value(*val, source_type, source);
		else if (const bool* val = std::get_if<bool>(&v))
			return set_value(*val, source_type, source);
		else if (const double* val = std::get_if<double>(&v))
			return set_value(*val, source_type, source);
		else if (const std::string* val = std::get_if<std::string>(&v))
			return set_value(*val, source_type, source);
		else
			throw new std::logic_error(fmt::format("{} param '{}' error: failed to get value from variant input arg", ParamUtils::GetApplicationName(), name_));
	}
#endif

	bool Param::can_update(ParamSetBySourceType source_type) const noexcept {
		switch (set_mode()) {
		case PARAM_VALUE_IS_DEFAULT:
		case PARAM_VALUE_IS_RESET:
			//
		case PARAM_VALUE_IS_SET_BY_APPLICATION:
		case PARAM_VALUE_IS_SET_BY_CORE_RUN:
			// apply always
			return true;;

		default:
			if (set_mode() < source_type)
				return true;

			// silently ignore this write attempt? :: order of precedence override.
			return false;
		}
	}

} // namespace tesseract
