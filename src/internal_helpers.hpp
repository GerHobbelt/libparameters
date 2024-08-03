
#pragma once

#include <parameters/parameter_classes.h>
#include <parameters/parameter_sets.h>

#include <type_traits>

namespace parameters {

#define THE_4_HANDLERS_PROTO                                                    \
  const char *name, const char *comment, ParamsVector &owner, bool init,        \
  ParamOnModifyFunction on_modify_f, ParamOnValidateFunction on_validate_f,     \
  ParamOnParseFunction on_parse_f, ParamOnFormatFunction on_format_f


	// -- local helper functions --

	using statistics_uint_t = decltype(Param::access_counts_t().reading);

	// increment value, prevent overflow, a.k.a. wrap-around, i.e. clip to maximum value
	template <class T, typename = std::enable_if_t<std::is_unsigned<T>::value>>
	static inline void safe_inc(T& sum) {
		sum++;
		// did a wrap-around just occur? if so, compensate by rewinding to max value.
		if (sum == T(0))
			sum--;
	}

	// add value to sum, prevent overflow, a.k.a. wrap-around, i.e. clip sum to maximum value
	template <class SumT, class ValT, typename = std::enable_if_t<std::is_unsigned<SumT>::value && std::is_unsigned<ValT>::value>>
	static inline void safe_add(SumT &sum, const ValT value) {
		// conditional check is shaped to work in overflow conditions
		if (sum < SumT(0) - 1 - value) // sum + value < max?  ==>  sum < max - value?
			sum += value;
		else                           // clip/limit ==>  sum = max
			sum = SumT(0) - 1;
	}

	// --- end of helper functions set ---


}   // namespace

