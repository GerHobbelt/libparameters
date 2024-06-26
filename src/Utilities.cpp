
#include <parameters/parameters.h>

#include "internal_helpers.hpp"

namespace parameters {


	static std::string params_appname_4_reporting = ParamUtils::GetApplicationName();

	// Set the application name to be mentioned in libparameters' error messages.
	void ParamUtils::SetApplicationName(const char* appname) {
		if (!appname || !*appname) {
			appname = "[?anonymous.app?]";

#if defined(_WIN32)
			{
				DWORD pathlen = MAX_PATH - 1;
				DWORD bufsize = pathlen + 1;
				LPSTR buffer = (LPSTR)malloc(bufsize * sizeof(buffer[0]));

				for (;;) {
					buffer[0] = 0;

					// On WinXP, if path length >= bufsize, the output is truncated and NOT
					// null-terminated.  On Vista and later, it will null-terminate the
					// truncated string. We call ReleaseBuffer on all OSes to be safe.
					pathlen = ::GetModuleFileNameA(NULL,
																				 buffer,
																				 bufsize);
					if (pathlen > 0 && pathlen < bufsize - 1)
						break;

					if (pathlen == 0 && ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
						buffer[0] = 0;
						break;
					}
					bufsize *= 2;
					buffer = (LPSTR)realloc(buffer, bufsize * sizeof(buffer[0]));
				}
				if (buffer[0]) {
					appname = buffer;
				}

				free(buffer);
			}
#endif
		}

		params_appname_4_reporting = appname;
	}

	const std::string& ParamUtils::GetApplicationName() {
		return params_appname_4_reporting;
	}

	// --------------------------------------------------------------------------------



	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// ParamUtils
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool ParamUtils::ReadParamsFile(const std::string &file,
																	const ParamsVectorSet &member_params,
															 SurplusParamsVector *surplus,
																	ParamSetBySourceType source_type,
																	ParamPtr source
	) {
		ConfigFile fp(file);
		if (!fp()) {
			tprintError("read_params_file: Can't open/read file {}\n", file);
			return true;
		}
		return ReadParamsFile(fp, member_params, surplus, source_type, source);
	}

	bool ParamUtils::ReadParamsFile(ConfigFile &fp,
																		const ParamsVectorSet &member_params,
															 SurplusParamsVector *surplus,
																		ParamSetBySourceType source_type,
																		ParamPtr source) {
		CString<4096> line; // input line
		bool anyerr = false;  // true if any error
		bool foundit;         // found parameter
		char *nameptr;        // name field
		char *valptr;         // value field
		unsigned linecounter = 0;

		while (fp.ReadLine(line)) {
			linecounter++;

			line.Trim();
			nameptr = line.data();

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
				foundit = SetParam(nameptr, valptr, member_params, source_type, source);

				if (!foundit) {
					anyerr = true; // had an error
					tprintError("Failure while processing parameter line #{}: {}  {}\n", linecounter, nameptr, valptr);
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
	StringSetParam *ParamUtils::FindParam<StringSetParam>(
			const char *name,
			const ParamsVectorSet &set) {
		return set.find<StringSetParam>(name);
	}

	template <>
	IntSetParam *ParamUtils::FindParam<IntSetParam>(
			const char *name,
			const ParamsVectorSet &set) {
		return set.find<IntSetParam>(name);
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
			const char *name, const int32_t value,
			const ParamsVectorSet &set,
			ParamSetBySourceType source_type, ParamPtr source) {
				{
					IntParam *param = FindParam<IntParam>(name, set);
					if (param != nullptr) {
						param->set_value(value, source_type, source);
						return !param->has_faulted();
					}
				}
				{
					Param *param = FindParam<Param>(name, set);
					if (param != nullptr) {
						switch (param->type()) {
						case INT_PARAM:
							DEBUG_ASSERT(0);
							break;

						case BOOL_PARAM: {
							BoolParam *bp = static_cast<BoolParam *>(param);
							bp->set_value(value != 0, source_type, source);
							return !bp->has_faulted();
						}

						case DOUBLE_PARAM: {
							DoubleParam *dp = static_cast<DoubleParam *>(param);
							dp->set_value(value, source_type, source);
							return !dp->has_faulted();
						}

						case STRING_PARAM:
						case CUSTOM_PARAM:
						case CUSTOM_SET_PARAM:
						default: {
							std::string vs = fmt::format("{}", value);
							param->set_value(vs, source_type, source);
							return !param->has_faulted();
						}

						case STRING_SET_PARAM: {
							std::vector<std::string> v;
							std::string vs = fmt::format("{}", value);
							v.push_back(vs);
							StringSetParam *p = static_cast<StringSetParam *>(param);
							p->set_value(v, source_type, source);
							return !p->has_faulted();
						}

						case INT_SET_PARAM: {
							std::vector<int32_t> iv;
							iv.push_back(value);
							IntSetParam *ivp = static_cast<IntSetParam *>(param);
							ivp->set_value(iv, source_type, source);
							return !ivp->has_faulted();
						}

						case BOOL_SET_PARAM: {
							std::vector<bool> bv;
							bv.push_back(value != 0);
							BoolSetParam *bvp = static_cast<BoolSetParam *>(param);
							bvp->set_value(bv, source_type, source);
							return !bvp->has_faulted();
						}

						case DOUBLE_SET_PARAM: {
							std::vector<double> dv;
							dv.push_back(value);
							DoubleSetParam *dvp = static_cast<DoubleSetParam *>(param);
							dvp->set_value(dv, source_type, source);
							return !dvp->has_faulted();
						}
						}
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
				param->set_value(value, source_type, source);
				return !param->has_faulted();
			}
		}
		{
			Param* param = FindParam<Param>(name, set);
			if (param != nullptr) {
				switch (param->type()) {
				case BOOL_PARAM:
					DEBUG_ASSERT(0);
					break;

				case INT_PARAM: {
					IntParam *bp = static_cast<IntParam *>(param);
					bp->set_value(value, source_type, source);
					return !bp->has_faulted();
				}

				case DOUBLE_PARAM: {
					DoubleParam *dp = static_cast<DoubleParam *>(param);
					dp->set_value(value, source_type, source);
					return !dp->has_faulted();
				}

				case STRING_PARAM:
				case CUSTOM_PARAM:
				case CUSTOM_SET_PARAM:
				default: {
					const char *vs = (value ? "true" : "false");
					param->set_value(vs, source_type, source);
					return !param->has_faulted();
				}

				case STRING_SET_PARAM: {
					std::vector<std::string> v;
					const char *vs = (value ? "true" : "false");
					v.push_back(vs);
					StringSetParam *p = static_cast<StringSetParam *>(param);
					p->set_value(v, source_type, source);
					return !p->has_faulted();
				}

				case INT_SET_PARAM: {
					std::vector<int32_t> iv;
					iv.push_back(value);
					IntSetParam *ivp = static_cast<IntSetParam *>(param);
					ivp->set_value(iv, source_type, source);
					return !ivp->has_faulted();
				}

				case BOOL_SET_PARAM: {
					std::vector<bool> bv;
					bv.push_back(value);
					BoolSetParam *bvp = static_cast<BoolSetParam *>(param);
					bvp->set_value(bv, source_type, source);
					return !bvp->has_faulted();
				}

				case DOUBLE_SET_PARAM: {
					std::vector<double> dv;
					dv.push_back(value);
					DoubleSetParam *dvp = static_cast<DoubleSetParam *>(param);
					dvp->set_value(dv, source_type, source);
					return !dvp->has_faulted();
				}
				}
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
				param->set_value(value, source_type, source);
				return !param->has_faulted();
			}
		}
		{
			Param* param = FindParam<Param>(name, set);
			if (param != nullptr) {
				switch (param->type()) {
				case DOUBLE_PARAM:
					DEBUG_ASSERT(0);
					break;

				case BOOL_PARAM: {
					BoolParam *bp = static_cast<BoolParam *>(param);
					// reckon with the inaccuracy/noise inherent in IEEE754 calculus.
					bool v = (value > -FLT_EPSILON && value < FLT_EPSILON);
					bp->set_value(v, source_type, source);
					return !bp->has_faulted();
				}

				case INT_PARAM: {
					IntParam *dp = static_cast<IntParam *>(param);
					auto v = round(value);
					if (v < INT32_MIN || v > INT32_MAX)
						return false;
					dp->set_value(int32_t(v), source_type, source);
					return !dp->has_faulted();
				}

				case STRING_PARAM:
				case CUSTOM_PARAM:
				case CUSTOM_SET_PARAM:
				default: {
					std::string vs = fmt::format("{}", value);
					param->set_value(vs, source_type, source);
					return !param->has_faulted();
				}

				case STRING_SET_PARAM: {
					std::vector<std::string> v;
					std::string vs = fmt::format("{}", value);
					v.push_back(vs);
					StringSetParam *p = static_cast<StringSetParam *>(param);
					p->set_value(v, source_type, source);
					return !p->has_faulted();
				}

				case INT_SET_PARAM: {
					std::vector<int32_t> iv;
					auto v = round(value);
					if (v < INT32_MIN || v > INT32_MAX)
						return false;
					iv.push_back(v);
					IntSetParam *ivp = static_cast<IntSetParam *>(param);
					ivp->set_value(iv, source_type, source);
					return !ivp->has_faulted();
				}

				case BOOL_SET_PARAM: {
					std::vector<bool> bv;
					// reckon with the inaccuracy/noise inherent in IEEE754 calculus.
					bool v = (value > -FLT_EPSILON && value < FLT_EPSILON);
					bv.push_back(v);
					BoolSetParam *bvp = static_cast<BoolSetParam *>(param);
					bvp->set_value(bv, source_type, source);
					return !bvp->has_faulted();
				}

				case DOUBLE_SET_PARAM: {
					std::vector<double> dv;
					dv.push_back(value);
					DoubleSetParam *dvp = static_cast<DoubleSetParam *>(param);
					dvp->set_value(dv, source_type, source);
					return !dvp->has_faulted();
				}
				}
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
				param->set_value(value, source_type, source);
				return !param->has_faulted();
			}
		}
		{
			Param* param = FindParam<Param>(name, set);
			if (param != nullptr) {
				param->set_value(value, source_type, source);
				return !param->has_faulted();
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
			param->set_value(value, source_type, source);
			return !param->has_faulted();
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
	
}  // namespace
