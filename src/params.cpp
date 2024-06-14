
#include <parameters/parameters.h>

#include <fmt/base.h>
#include <fmt/format.h>

#include <climits> // for INT_MIN, INT_MAX
#include <cmath>   // for NAN, std::isnan
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <locale>  // for std::locale::classic
#include <sstream> // for std::stringstream
#include <functional>
#include <exception>
#include <cctype>  // for std::toupper

#if defined(HAVE_MUPDF)
#include "mupdf/assertions.h"
#endif

#ifdef _WIN32
#  include <windows.h>
#  define strcasecmp _stricmp
#  define strncasecmp _strnicmp
#else
#  include <strings.h>
#endif


namespace parameters {

		ParamsVector &GlobalParams() {
		static ParamsVector global_params("global"); // static auto-inits at startup
		return global_params;
	}

	// -- local helper functions --

	static inline bool strieq(const char *s1, const char *s2) {
		return strcasecmp(s1, s2) == 0;
	}

	static bool SafeAtoi(const char* str, int* val) {
		char* endptr = nullptr;
		*val = strtol(str, &endptr, 10);
		return endptr != nullptr && *endptr == '\0';
	}

	static bool is_legal_fpval(double val) {
		return !std::isnan(val) && val != HUGE_VAL;
	}

	static bool SafeAtod(const char* str, double* val) {
		char* endptr = nullptr;
		double d = NAN;
		std::stringstream stream(str);
		// Use "C" locale for reading double value.
		stream.imbue(std::locale::classic());
		stream >> d;
		*val = 0;
		bool success = is_legal_fpval(d);
		if (success) {
			*val = d;
		}
		return success;
	}

	static bool is_single_word(const char* s) {
		if (!*s)
			return false;
		while (isalpha(*s))
			s++;
		while (isspace(*s))
			s++;
		return (!*s); // string must be at the end now...
	}

	// --- end of helper functions set ---




	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamHash
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Note about Param names, i.e. Variable Names:
	// 
	// - accept both `-` and `_` in key names, e.g. user-specified 'debug-all' would match 'debug_all'
	//   in the database.
	// - names are matched case-*in*sensitive and must be ASCII. Unicode characters in Variable Names 
	//   are not supported.

	// calculate hash:
	std::size_t ParamHash::operator()(const Param& s) const noexcept {
		const char * str = s.name_str();
		return ParamHash()(str);
	}
	// calculate hash:
	std::size_t ParamHash::operator()(const char * s) const noexcept {
		ASSERT0(s != nullptr);
		uint32_t h = 1;
		for (const char *p = s; *p; p++) {
			uint32_t c = std::toupper(static_cast<unsigned char>(*p));
			if (c == '-')
				c = '_';
			h *= 31397;
			h += c;
		}
		return h;
	}

	// equal_to:
	bool ParamHash::operator()(const Param& lhs, const Param& rhs) const noexcept {
		ASSERT0(lhs.name_str() != nullptr);
		ASSERT0(rhs.name_str() != nullptr);
		return ParamHash()(lhs.name_str(), rhs.name_str());
	}
	// equal_to:
	bool ParamHash::operator()(const char * lhs, const char * rhs) const noexcept {
		ASSERT0(lhs != nullptr);
		ASSERT0(rhs != nullptr);
		for (; *lhs; lhs++, rhs++) {
			uint32_t c = std::toupper(static_cast<unsigned char>(*lhs));
			if (c == '-')
				c = '_';
			uint32_t d = std::toupper(static_cast<unsigned char>(*rhs));
			if (d == '-')
				d = '_';
			if (c != d)
				return false;
		}
		ASSERT0(*lhs == 0);
		return *rhs == 0;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamComparer
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// compare as a-less-b? for purposes of std::sort et al:
	bool ParamComparer::operator()(const Param& lhs, const Param& rhs) const {
		return ParamComparer()(lhs.name_str(), rhs.name_str());
	}
	// compare as a-less-b? for purposes of std::sort et al:
	bool ParamComparer::operator()(const char * lhs, const char * rhs) const {
		ASSERT0(lhs != nullptr);
		ASSERT0(rhs != nullptr);
		for (; *lhs; lhs++, rhs++) {
			uint32_t c = std::toupper(static_cast<unsigned char>(*lhs));
			if (c == '-')
				c = '_';
			uint32_t d = std::toupper(static_cast<unsigned char>(*rhs));
			if (d == '-')
				d = '_';
			// long names come before short names; otherwise sort A->Z
			if (c != d)
				return d == 0 ? true : (c < d);
		}
		ASSERT0(*lhs == 0);
		return *rhs == 0;
	}


#ifndef NDEBUG
	static void check_and_report_name_collisions(const char *name, const ParamsHashTableType &table) {
		if (table.contains(name)) {
			std::string s = fmt::format("tesseract param name '{}' collision: double definition of param '{}'", name, name);
			throw new std::logic_error(s);
		}
	}
	static void check_and_report_name_collisions(const char *name, std::vector<ParamPtr> &table) {
		for (Param *p : table) {
			if (ParamHash()(p->name_str(), name)) {
				std::string s = fmt::format("tesseract param name '{}' collision: double definition of param '{}'", name, name);
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

	void ParamsVector::remove(ParamPtr param_ref) {
		params_.erase(param_ref->name_str());
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


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// Param
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

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
			throw new std::logic_error(fmt::format("tesseract param '{}' error: failed to get value from variant input arg", name_));
	}

	ParamOnModifyFunction Param::set_on_modify_handler(ParamOnModifyFunction on_modify_f) {
		ParamOnModifyFunction rv = on_modify_f_;
		on_modify_f_ = on_modify_f;
		return rv;
	}
	ParamOnModifyFunction Param::clear_on_modify_handler() {
		return set_on_modify_handler(0);
	}

	const char* Param::name_str() const {
		return name_;
	}
	const char* Param::info_str() const {
		return info_;
	}
	bool Param::is_init() const {
		return init_;
	}
	bool Param::is_debug() const {
		return debug_;
	}

	ParamSetBySourceType Param::set_mode() const {
		return set_mode_;
	}
	Param* Param::is_set_by() const {
		return setter_;
	}

	ParamsVector& Param::owner() const {
		return owner_;
	}

	Param::access_counts_t Param::access_counts() const {
		return access_counts_;
	}

	void Param::reset_access_counts() {
		access_counts_.prev_sum_reading += access_counts_.reading;
		access_counts_.prev_sum_writing += access_counts_.writing;
		access_counts_.prev_sum_changing += access_counts_.changing;

		access_counts_.reading = 0;
		access_counts_.writing = 0;
		access_counts_.changing = 0;
	}

	bool Param::set_value(const std::string& v, ParamSetBySourceType source_type, ParamPtr source) {
		return set_value(v.c_str(), source_type, source);
	}

	ParamType Param::type() const {
		return type_;
	}

	Param::Param(const char* name, const char* comment, ParamsVector& owner, bool init, ParamOnModifyFunction on_modify_f):
		owner_(owner),
		name_(name),
		info_(comment),
		init_(init),
		type_(UNKNOWN_PARAM),
		set_mode_(PARAM_VALUE_IS_DEFAULT),
		setter_(nullptr),
		on_modify_f_(on_modify_f),
		access_counts_({0,0,0})
	{
		debug_ = (strstr(name, "debug") != nullptr) || (strstr(name, "display") != nullptr);

		owner.add(this);
	}




	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// IntParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IntParam::set_value(int32_t value, ParamSetBySourceType source_type, ParamPtr source) {
		access_counts_.writing++;
		if (value != value_ && value != default_)
			access_counts_.changing++;

		if (!!on_modify_f_) {
			ParamValueContainer old(value_);
			ParamValueContainer now(value);

			value_ = value;

			on_modify_f_(name_, *this, source_type, source, old, now);

			if (const int32_t* val = std::get_if<int32_t>(&now)) {
				value_ = *val;
			}
		} else {
			value_ = value;
		}
		return true;
	}

	bool IntParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		int32_t val = 0;
		return SafeAtoi(v, &val) && set_value(val, source_type, source);
	}
	bool IntParam::set_value(bool v, ParamSetBySourceType source_type, ParamPtr source) {
		int32_t val = !!v;
		return set_value(val, source_type, source);
	}
	bool IntParam::set_value(double v, ParamSetBySourceType source_type, ParamPtr source) {
		v = roundf(v);
		if (v < INT32_MIN || v > INT32_MAX)
			return false;

		int32_t val = v;
		return set_value(val, source_type, source);
	}

	IntParam::IntParam(int32_t value, const char* name, const char* comment, ParamsVector& owner, bool init, ParamOnModifyFunction on_modify_f)
		: Param(name, comment, owner, init, on_modify_f) {
		value_ = value;
		default_ = value;
		type_ = INT_PARAM;
	}

	IntParam::operator int32_t() const {
		access_counts_.reading++;
		return value_;
	}
	void IntParam::operator=(int32_t value) {
		set_value(value, PARAM_VALUE_IS_SET_BY_ASSIGN);
	}


	int32_t IntParam::value() const {
		access_counts_.reading++;
		return value_;
	}

	void IntParam::ResetToDefault(ParamSetBySourceType source_type) {
		set_value(default_, source_type, nullptr);
	}

	void IntParam::ResetFrom(const ParamsVectorSet& vec, ParamSetBySourceType source_type) {
		IntParam* param = vec.find<IntParam>(name_);
		if (param) {
			set_value(*param, source_type, param);
		} else {
			ResetToDefault(source_type);
		}
	}

	std::string IntParam::formatted_value_str() const {
		return std::to_string(value_);
	}

	const char* IntParam::value_type_str() const {
		return "integer";
	}

	std::string IntParam::raw_value_str() const {
		return std::to_string(value_);
	}

	bool IntParam::inspect_value(ParamValueContainer & dst) const {
		return false;
	}





	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// BoolParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool BoolParam::set_value(bool value, ParamSetBySourceType source_type, ParamPtr source) {
		access_counts_.writing++;
		if (value != value_ && value != default_)
			access_counts_.changing++;

		if (!!on_modify_f_) {
			ParamValueContainer old(value_);
			ParamValueContainer now(value);

			value_ = value;

			on_modify_f_(name_, *this, source_type, source, old, now);

			if (const bool* val = std::get_if<bool>(&now)) {
				value_ = *val;
			}
		} else {
			value_ = value;
		}
		return true;
	}

	bool BoolParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		int32_t val = 0;
		if (!SafeAtoi(v, &val)) {
			while (isspace(*v))
				v++;
			switch (tolower(v[0])) {
			case 't':
				// true; only valid when a single char or word:
				if (!is_single_word(v))
					return false;
				val = 1;
				break;

			case 'f':
				// false; only valid when a single char or word:
				if (!is_single_word(v))
					return false;
				val = 0;
				break;

			case 'y':
			case 'j':
				// yes / ja; only valid when a single char or word:
				if (!is_single_word(v))
					return false;
				val = 1;
				break;

			case 'n':
				// no; only valid when a single char or word:
				if (!is_single_word(v))
					return false;
				val = 0;
				break;

			case 'x':
				// on; only valid when alone:
				if (v[1])
					return false;
				val = 1;
				break;

			case '-':
			case '.':
				// off; only valid when alone:
				if (v[1])
					return false;
				val = 0;
				break;

			default:
				return false;
			}
		}
		bool b = (val != 0);
		return set_value(b, source_type, source);
	}

	bool BoolParam::set_value(int32_t v, ParamSetBySourceType source_type, ParamPtr source) {
		bool val = (v != 0);
		return set_value(val, source_type, source);
	}

	// based on https://stackoverflow.com/questions/13698927/compare-double-to-zero-using-epsilon
#define inline_constexpr   inline
	static inline_constexpr double epsilon_plus()
	{
		const double a = 0.0;
		return std::nextafter(a, std::numeric_limits<double>::max());
	}
	static inline_constexpr double epsilon_minus()
	{
		const double a = 0.0;
		return std::nextafter(a, std::numeric_limits<double>::lowest());
	}
	static bool is_zero(const double b)
	{
		return epsilon_minus() <= b
			&& epsilon_plus() >= b;
	}

	bool BoolParam::set_value(double v, ParamSetBySourceType source_type, ParamPtr source) {
		bool zero = is_zero(v);

		return set_value(!zero, source_type, source);
	}

	BoolParam::BoolParam(bool value, const char* name, const char* comment, ParamsVector& owner, bool init, ParamOnModifyFunction on_modify_f)
		: Param(name, comment, owner, init, on_modify_f) {
		value_ = value;
		default_ = value;
		type_ = BOOL_PARAM;
	}

	BoolParam::operator bool() const {
		access_counts_.reading++;
		return value_;
	}

	void BoolParam::operator=(bool value) {
		access_counts_.writing++;
		if (value != value_ && value != default_)
			access_counts_.changing++;
		value_ = value;
	}


	bool BoolParam::value() const {
		access_counts_.reading++;
		return value_;
	}

	void BoolParam::ResetToDefault(ParamSetBySourceType source_type) {
		set_value(default_, source_type, nullptr);
	}

	void BoolParam::ResetFrom(const ParamsVectorSet& vec, ParamSetBySourceType source_type) {
		BoolParam* param = vec.find<BoolParam>(name_);
		if (param) {
			set_value(*param, source_type, param);
		} else {
			ResetToDefault(source_type);
		}
	}

	std::string BoolParam::formatted_value_str() const {
		return value_ ? "true" : "false";
	}

	const char* BoolParam::value_type_str() const {
		return "boolean";
	}

	std::string BoolParam::raw_value_str() const {
		return value_ ? "true": "false";
	}

	bool BoolParam::inspect_value(ParamValueContainer& dst) const {
		return false;
	}


	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// DoubleParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DoubleParam::set_value(double value, ParamSetBySourceType source_type, ParamPtr source) {
		if (!is_legal_fpval(value))
			return false;

		access_counts_.writing++;
		if (value != value_ && value != default_)
			access_counts_.changing++;

		if (!!on_modify_f_) {
			ParamValueContainer old(value_);
			ParamValueContainer now(value);

			value_ = value;

			on_modify_f_(name_, *this, source_type, source, old, now);

			if (const double* val = std::get_if<double>(&now)) {
				value_ = *val;
			}
		} else {
			value_ = value;
		}
		return true;
	}

	bool DoubleParam::set_value(const char *v, ParamSetBySourceType source_type, ParamPtr source) {
		double val = 0.0;
		return SafeAtod(v, &val) && set_value(val, source_type, source);
	}
	bool DoubleParam::set_value(bool v, ParamSetBySourceType source_type, ParamPtr source) {
		double val = !!v;
		return set_value(val, source_type, source);
	}
	bool DoubleParam::set_value(int32_t v, ParamSetBySourceType source_type, ParamPtr source) {
		double val = v;
		return set_value(val, source_type, source);
	}

	DoubleParam::DoubleParam(double value, const char* name, const char* comment, ParamsVector& owner, bool init, ParamOnModifyFunction on_modify_f)
		: Param(name, comment, owner, init, on_modify_f) {
		value_ = value;
		default_ = value;
		type_ = DOUBLE_PARAM;
	}

	DoubleParam::operator double() const {
		access_counts_.reading++;
		return value_;
	}
	void DoubleParam::operator=(double value) {
		access_counts_.writing++;
		if (value != value_ && value != default_)
			access_counts_.changing++;
		value_ = value;
	}


	double DoubleParam::value() const {
		access_counts_.reading++;
		return value_;
	}

	void DoubleParam::ResetToDefault(ParamSetBySourceType source_type) {
		set_value(default_, source_type, nullptr);
	}
	void DoubleParam::ResetFrom(const ParamsVectorSet& vec, ParamSetBySourceType source_type) {
		DoubleParam* param = vec.find<DoubleParam>(name_);
		if (param) {
			set_value(*param, source_type, param);
		} else {
			ResetToDefault(source_type);
		}
	}

	std::string DoubleParam::formatted_value_str() const {
#if 0
		return std::to_string(value_);   // always outputs %.6f format style values
#else
		char sbuf[40];
		snprintf(sbuf, sizeof(sbuf), "%1.f", value_);
		sbuf[39] = 0;
		return sbuf;
#endif
	}

	const char* DoubleParam::value_type_str() const {
		return "floating point";
	}

	std::string DoubleParam::raw_value_str() const {
#if 0
		return std::to_string(value_);   // always outputs %.6f format style values
#else
		char sbuf[40];
		snprintf(sbuf, sizeof(sbuf), "%1.f", value_);
		sbuf[39] = 0;
		return sbuf;
#endif
	}

	bool DoubleParam::inspect_value(ParamValueContainer& dst) const {
		return false;
	}



	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// StringParam
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool StringParam::set_value(const char* value, ParamSetBySourceType source_type, ParamPtr source) {
		if (value == nullptr)
			value = "";

		access_counts_.writing++;
		if (value != value_ && value != default_)
			access_counts_.changing++;

		if (!!on_modify_f_) {
			ParamValueContainer old(value_);
			ParamValueContainer now(value);

			value_ = value;

			on_modify_f_(name_, *this, source_type, source, old, now);

			if (const std::string* val = std::get_if<std::string>(&now)) {
				value_ = *val;
			}
		} else {
			value_ = value;
		}
		return true;
	}

	bool StringParam::set_value(int32_t v, ParamSetBySourceType source_type, ParamPtr source) {
		std::string val = std::to_string(v);
		return set_value(val.c_str(), source_type, source);
	}
	bool StringParam::set_value(bool v, ParamSetBySourceType source_type, ParamPtr source) {
		const char *val = (v ? "true" : "false");
		return set_value(val, source_type, source);
	}
	bool StringParam::set_value(double v, ParamSetBySourceType source_type, ParamPtr source) {
		std::string val = std::to_string(v);
		return set_value(val.c_str(), source_type, source);
	}

	StringParam::StringParam(const char* value, const char* name, const char* comment, ParamsVector& owner, bool init, ParamOnModifyFunction on_modify_f)
		: Param(name, comment, owner, init, on_modify_f) {
		value_ = value;
		default_ = value;
		type_ = STRING_PARAM;
	}

	StringParam::operator std::string& () {
		access_counts_.reading++;
		return value_;
	}
	const char* StringParam::c_str() const {
		access_counts_.reading++;
		return value_.c_str();
	}
	bool StringParam::contains(char c) const {
		access_counts_.reading++;
		return value_.find(c) != std::string::npos;
	}
	bool StringParam::empty() const {
		access_counts_.reading++;
		return value_.empty();
	}
	bool StringParam::operator==(const std::string& other) const {
		access_counts_.reading++;
		return value_ == other;
	}
	void StringParam::operator=(const std::string& value) {
		access_counts_.writing++;
		if (value != value_ && value != default_)
			access_counts_.changing++;
		value_ = value;
	}


	const std::string& StringParam::value() const {
		access_counts_.reading++;
		return value_;
	}

	void StringParam::ResetToDefault(ParamSetBySourceType source_type) {
		const std::string& v = default_;
		(void)Param::set_value(v, source_type, nullptr);
	}

	void StringParam::ResetFrom(const ParamsVectorSet& vec, ParamSetBySourceType source_type) {
		StringParam* param = vec.find<StringParam>(name_);
		if (param) {
			set_value(param, source_type, param);
		} else {
			ResetToDefault(source_type);
		}
	}

	std::string StringParam::formatted_value_str() const {
		std::string rv = "\u00AB";
		rv += value_;
		rv += "\u00BB";
		return std::move(rv);
	}

	const char* StringParam::value_type_str() const {
		return "string";
	}

	std::string StringParam::raw_value_str() const {
		std::string rv(value_);
		return std::move(rv);
	}

	bool StringParam::inspect_value(ParamValueContainer& dst) const {
		return false;
	}



	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamUtils
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool ParamUtils::ReadParamsFile(const std::string &file,
																	const ParamsVectorSet &member_params,
																	ParamSetBySourceType source_type,
																	ParamPtr source
	) {
		TFile fp;
		if (!fp.Open(file.c_str(), nullptr)) {
			tprintError("read_params_file: Can't open/read file {}\n", file);
			return true;
		}
		return ReadParamsFromFp(&fp, member_params, source_type, source);
	}

	bool ParamUtils::ReadParamsFromFp(TFile *fp,
									const ParamsVectorSet& member_params,
									ParamSetBySourceType source_type,
									ParamPtr source
	) {
#define LINE_SIZE 4096
		char line[LINE_SIZE]; // input line
		bool anyerr = false;  // true if any error
		bool foundit;         // found parameter
		char *nameptr;        // name field
		char *valptr;         // value field
		unsigned linecounter = 0;

		while (fp->FGets(line, LINE_SIZE) != nullptr) {
			linecounter++;

			// trimRight:
			for (nameptr = line + strlen(line) - 1; nameptr >= line && std::isspace(*nameptr); nameptr--) {
				;
			}
			nameptr[1] = 0;
			// trimLeft:
			for (nameptr = line; *nameptr && std::isspace(*nameptr); nameptr++) {
				;
			}

			if (nameptr[0] && nameptr[0] != '#') {
				// jump over variable name
				for (valptr = nameptr; *valptr && !std::isspace(*valptr); valptr++) {
					;
				}

				if (*valptr) {    // found blank
					*valptr = '\0'; // make name a string

					do {
						valptr++; // find end of blanks
					} while (std::isspace(*valptr));
				}
				foundit = SetParam((const char *)nameptr, (const char*)valptr, member_params, source_type, source);

				if (!foundit) {
					anyerr = true; // had an error
					tprintError("Failure while processing parameter line: {}  {}\n", nameptr, valptr);
				}
			}
		}
		return anyerr;
	}

	template <>
	IntParam* ParamUtils::FindParam<IntParam>(
			const char* name,
			const ParamsVectorSet& set
	) {
		return set.find<IntParam>(name);
	}

	template <>
	BoolParam* ParamUtils::FindParam<BoolParam>(
			const char* name,
			const ParamsVectorSet& set
	) {
		return set.find<BoolParam>(name);
	}

	template <>
	DoubleParam* ParamUtils::FindParam<DoubleParam>(
			const char* name,
			const ParamsVectorSet& set
	) {
		return set.find<DoubleParam>(name);
	}

	template <>
	StringParam* ParamUtils::FindParam<StringParam>(
			const char* name,
			const ParamsVectorSet& set
	) {
		return set.find<StringParam>(name);
	}

	template <>
	Param* ParamUtils::FindParam<Param>(
			const char* name,
			const ParamsVectorSet& set
	) {
		return set.find<Param>(name);
	}

	Param* ParamUtils::FindParam(
		const char* name,
		const ParamsVectorSet& set,
		ParamType accepted_types_mask
	) {
		return set.find(name, accepted_types_mask);
	}


#if 0
	template <ParamDerivativeType T>
	T* ParamUtils::FindParam(
		const char* name,
		const ParamsVector& set
	) {
		ParamsVectorSet pvec({&set});

		return FindParam<T>(
			name,
			pvec
		);
	}
#endif


	Param* ParamUtils::FindParam(
		const char* name,
		const ParamsVector& set,
		ParamType accepted_types_mask
	) {
		ParamsVectorSet pvec;
		const ParamsVector* set_ptr = &set;
		pvec.add(const_cast<ParamsVector*>(set_ptr));

		return FindParam(
			name,
			pvec,
			accepted_types_mask
		);
	}


	template <>
	bool ParamUtils::SetParam<int32_t>(
			const char* name, const int32_t value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source
	) {
		{
			IntParam* param = FindParam<IntParam>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		{
			Param* param = FindParam<Param>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		return false;
	}

	template <>
	bool ParamUtils::SetParam<bool>(
			const char* name, const bool value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source
	) {
		{
			BoolParam* param = FindParam<BoolParam>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		{
			Param* param = FindParam<Param>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		return false;
	}

	template <>
	bool ParamUtils::SetParam<double>(
			const char* name, const double value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source
	) {
		{
			DoubleParam* param = FindParam<DoubleParam>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		{
			Param* param = FindParam<Param>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		return false;
	}

	bool ParamUtils::SetParam(
			const char* name, const std::string &value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source
	) {
		{
			StringParam* param = FindParam<StringParam>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		{
			Param* param = FindParam<Param>(name, set);
			if (param != nullptr) {
				return param->set_value(value, source_type, source);
			}
		}
		return false;
	}

	bool ParamUtils::SetParam(
			const char* name, const char *value,
			const ParamsVectorSet& set,
			ParamSetBySourceType source_type, ParamPtr source
	) {
		Param* param = FindParam(name, set, ANY_TYPE_PARAM);
		if (param != nullptr) {
			return param->set_value(value, source_type, source);
		}
		return false;
	}


#if 0
	template <ParamAcceptableValueType T>
	bool ParamUtils::SetParam(
		const char* name, const T value,
		ParamsVector& set,
		ParamSetBySourceType source_type, ParamPtr source
	) {
		ParamsVectorSet pvec({&set});
		return SetParam<T>(name, value, pvec, source_type, source);
	}
#endif


	bool ParamUtils::SetParam(
		const char* name, const char* value,
		ParamsVector& set,
		ParamSetBySourceType source_type, ParamPtr source
	) {
		ParamsVectorSet pvec({&set});
		return SetParam(name, value, pvec, source_type, source);
	}


	bool ParamUtils::InspectParamAsString(
			std::string* value_ref, const char* name,
			const ParamsVectorSet& set,
			ParamType accepted_types_mask
	) {
		return false;
	}

	bool ParamUtils::InspectParamAsString(
			std::string* value_ref, const char* name,
			const ParamsVector& set,
			ParamType accepted_types_mask
	) {
		return false;
	}

	bool ParamUtils::InspectParam(
			ParamValueContainer& value_dst, const char* name,
			const ParamsVectorSet& set,
			ParamType accepted_types_mask
	) {
		return false;
	}

	bool ParamUtils::InspectParam(
			ParamValueContainer& value_dst, const char* name,
			const ParamsVector& set,
			ParamType accepted_types_mask
	) {
		return false;
	}

	void ParamUtils::PrintParams(FILE* fp, const ParamsVectorSet& set, bool print_info) {

	}

	void ParamUtils::ReportParamsUsageStatistics(FILE* fp, const ParamsVectorSet& set, const char* section_title) {

	}

	void ParamUtils::ResetToDefaults(const ParamsVectorSet& set, ParamSetBySourceType source_type) {

	}






	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ReportFile
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	// permanent lookup table:
	std::vector<std::string> ReportFile::_processed_file_paths;


	ReportFile::ReportFile(const char *path)
	{
		if (!path || !*path) {
			_f = nullptr;
			return;
		}

		_f = nullptr;

		if (strieq(path, "/dev/stdout") || strieq(path, "stdout") || strieq(path, "-") || strieq(path, "1"))
			_f = stdout;
		else if (strieq(path, "/dev/stderr") || strieq(path, "stderr") || strieq(path, "+") || strieq(path, "2"))
			_f = stderr;
		else {
			bool first = true;
			for (std::string &i : _processed_file_paths) {
				if (strieq(i.c_str(), path)) {
					first = false;
					break;
				}
			}
			_f = fopenUtf8(path, first ? "w" : "a");
			if (!_f) {
				tprintError("Cannot produce parameter usage report file: '{}'\n", path);
			} else if (first) {
				_processed_file_paths.push_back(path);
			}
		}
	}

	ReportFile::~ReportFile() {
		if (_f) {
			if (_f != stdout && _f != stderr) {
				fclose(_f);
			} else {
				fflush(_f);
			}
		}
	}

	FILE * ReportFile::operator()() const {
		return _f;
	}





	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamsReportWriter, et al
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	class ParamsReportWriter {
	public:
		ParamsReportWriter(FILE *f)
			: file_(f) {}
		virtual ~ParamsReportWriter() = default;

		virtual void Write(const std::string message) = 0;

	protected:
		FILE *file_;
	};

	class ParamsReportDefaultWriter: public ParamsReportWriter {
	public:
		ParamsReportDefaultWriter(): ParamsReportWriter(nullptr) {}
		virtual ~ParamsReportDefaultWriter() = default;

		virtual void Write(const std::string message) {
			tprintDebug("{}", message);
		}

	protected:
	};

	class ParamsReportFileDuoWriter: public ParamsReportWriter {
	public:
		ParamsReportFileDuoWriter(FILE *f): ParamsReportWriter(f) {
			is_separate_file_ = (f != nullptr && f != stderr && f != stdout);
		}
		virtual ~ParamsReportFileDuoWriter() = default;

		virtual void Write(const std::string message) {
			// only write via tprintDebug() -- which usually logs to stderr -- when the `f` file destination is an actual file, rather than stderr or stdout.
			// This prevents these report lines showing up in duplicate on the console.
			if (is_separate_file_) {
				tprintDebug("{}", message);
			}
			size_t len = message.length();
			if (fwrite(message.c_str(), 1, len, file_) != len) {
				tprintError("Failed to write params-report line to file. {}\n", strerror(errno));
			}
		}

	protected:
		bool is_separate_file_;
	};


	static inline const char *type_as_str(ParamType type) {
		switch (type) {
		case INT_PARAM:
			return "[Integer]";
		case BOOL_PARAM:
			return "[Boolean]";
		case DOUBLE_PARAM:
			return "[Float]";
		case STRING_PARAM:
			return "[String]";
		case ANY_TYPE_PARAM:
			return "[ANY]";
		default:
			return "[???]";
		}
	}




#if 0

	// When `section_title` is NULL, this will report the lump sum parameter usage for the entire run.
	// When `section_title` is NOT NULL, this will only report the parameters that were actually used (R/W) during the last section of the run, i.e.
	// since the previous invocation of this reporting method (or when it hasn't been called before: the start of the application).
	void ParamUtils::ReportParamsUsageStatistics(FILE *f, const ParamsVectorSet *member_params, const char *section_title)
	{
		bool is_section_subreport = (section_title != nullptr);

		std::unique_ptr<ParamsReportWriter> writer;

		if (f != nullptr) {
			writer.reset(new ParamsReportFileDuoWriter(f));
		} else {
			writer.reset(new ParamsReportDefaultWriter());
		}

		writer->Write(fmt::format("\n\nTesseract Parameter Usage Statistics{}: which params have been relevant?\n"
			"----------------------------------------------------------------------\n\n",
			(section_title != nullptr ? fmt::format(" for section: {}", section_title) : "")));

		// first collect all parameters and sort them according to these criteria:
		// - global / (class)local
		// - name

		const ParamsVector* globals = GlobalParams();

		struct ParamInfo {
			Param *p;
			bool global;
		};

		std::vector<ParamInfo> params;
		{
			std::vector<Param *> ll = globals->as_list();
			params.reserve(ll.size());
			for (Param *i : ll) {
				params.push_back({i, true});
			}
		}

		if (member_params != nullptr) {
			std::vector<Param *> ll = member_params->as_list();
			params.reserve(ll.size() + params.size());
			for (Param *i : ll) {
				params.push_back({i, false});
			}
		}

		sort(params.begin(), params.end(), [](ParamInfo& a, ParamInfo& b)
		{
				int rv = (int)a.global - (int)b.global;
				if (rv == 0)
				{
					rv = (int)a.p->is_init() - (int)b.p->is_init();
					if (rv == 0)
					{
						rv = (int)b.p->is_debug() - (int)a.p->is_debug();
						if (rv == 0)
						{
							rv = strcmp(b.p->name_str(), a.p->name_str());
#if !defined(NDEBUG)
							if (rv == 0)
							{
								fprintf(stderr, "Apparently you have double-defined Tesseract Variable: '%s'! Fix that in the source code!\n", a.p->name_str());
								ASSERT0(!"Apparently you have double-defined a Tesseract Variable.");
							}
#endif
						}
					}
				}
				return (rv >= 0);
		});

		static const char* categories[] = {"(Global)", "(Local)"};
		static const char* sections[] = {"", "(Init)", "(Debug)", "(Init+Dbg)"};
		static const char* write_access[] = {".", "w", "W"};
		static const char* read_access[] = {".", "r", "R"};

		auto acc = [](int access) {
			if (access > 2)
				access = 2;
			return access;
			};

		if (!is_section_subreport) {
			// produce the final lump-sum overview report

			for (ParamInfo &item : params) {
				Param *p = item.p;
				p->reset_access_counts();
			}

			for (ParamInfo &item : params) {
				const Param* p = item.p;
				auto stats = p->access_counts();
				if (stats.prev_sum_reading > 0)
				{
					int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
					writer->Write(fmt::format("* {:.<60} {:8}{:10} {}{} {:9} = {}\n", p->name_str(), categories[item.global], sections[section], write_access[acc(stats.prev_sum_writing)], read_access[acc(stats.prev_sum_reading)], type_as_str(p->type()), p->formatted_value_str()));
				}
			}

			if (report_all_variables)
			{
				writer->Write("\n\nUnused parameters:\n\n");

				for (ParamInfo &item : params) {
					const Param* p = item.p;
					auto stats = p->access_counts();
					if (stats.prev_sum_reading <= 0)
					{
						int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
						writer->Write(fmt::format("* {:.<60} {:8}{:10} {}{} {:9} = {}\n", p->name_str(), categories[item.global], sections[section], write_access[acc(stats.prev_sum_writing)], read_access[acc(stats.prev_sum_reading)], type_as_str(p->type()), p->formatted_value_str()));
					}
				}
			}
		} else {
			// produce the section-local report of used parameters

			for (ParamInfo &item : params) {
				const Param* p = item.p;
				auto stats = p->access_counts();
				if (stats.reading > 0)
				{
					int section = ((int)p->is_init()) | (2 * (int)p->is_debug());
					writer->Write(fmt::format("* {:.<60} {:8}{:10} {}{} {:9} = {}\n", p->name_str(), categories[item.global], sections[section], write_access[acc(stats.prev_sum_writing)], read_access[acc(stats.prev_sum_reading)], type_as_str(p->type()), p->formatted_value_str()));
				}
			}

			// reset the access counts for the next section:
			for (ParamInfo &item : params) {
				Param* p = item.p;
				p->reset_access_counts();
			}
		}
	}

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

	void ParamUtils::PrintParams(FILE *fp, const ParamsVectorSet *member_params, bool print_info) {
		int num_iterations = (member_params == nullptr) ? 1 : 2;
		// When printing to stdout info text is included.
		// Info text is omitted when printing to a file (would result in an invalid config file).
		if (!fp)
			fp = stdout;
		bool printing_to_stdio = (fp == stdout || fp == stderr);
		std::ostringstream stream;
		stream.imbue(std::locale::classic());
		for (int v = 0; v < num_iterations; ++v) {
			const ParamsVectorSet *vec = (v == 0) ? GlobalParams() : member_params;
			for (auto int_param : vec->int_params_c()) {
				if (print_info) {
					stream << int_param->name_str() << '\t' << (int32_t)(*int_param) << '\t'
						<< int_param->info_str() << '\n';
				} else {
					stream << int_param->name_str() << '\t' << (int32_t)(*int_param) << '\n';
				}
			}
			for (auto bool_param : vec->bool_params_c()) {
				if (print_info) {
					stream << bool_param->name_str() << '\t' << bool(*bool_param) << '\t'
						<< bool_param->info_str() << '\n';
				} else {
					stream << bool_param->name_str() << '\t' << bool(*bool_param) << '\n';
				}
			}
			for (auto string_param : vec->string_params_c()) {
				if (print_info) {
					stream << string_param->name_str() << '\t' << string_param->c_str() << '\t'
						<< string_param->info_str() << '\n';
				} else {
					stream << string_param->name_str() << '\t' << string_param->c_str() << '\n';
				}
			}
			for (auto double_param : vec->double_params_c()) {
				if (print_info) {
					stream << double_param->name_str() << '\t' << (double)(*double_param) << '\t'
						<< double_param->info_str() << '\n';
				} else {
					stream << double_param->name_str() << '\t' << (double)(*double_param) << '\n';
				}
			}
		}
		if (printing_to_stdio)
		{
			tprintDebug("{}", stream.str().c_str());
		} else
		{
			fprintf(fp, "%s", stream.str().c_str());
		}
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






















































#endif


} // namespace tesseract
