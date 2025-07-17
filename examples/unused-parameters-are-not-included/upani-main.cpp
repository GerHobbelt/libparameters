
// showcase the scenario where parameters, which
// are otherwise unused in the application, 
// will be discarded at the linker phase.
//
// this can be observed by inspecting the list of registered parameters,
// as produced by the libparameters manager class/singleton.
//



#include <parameters/parameters.h>

#include "var-a-b-c-d.hpp"

using namespace parameters;

#include <iostream>
#include <format>
#include <typeinfo>
#include <type_traits>

#include "monolithic_examples.h"

#if defined(BUILD_MONOLITHIC)
#ifndef SINGLE_SOURCE_DEMO
#define main param_unused_params_example_A_main
#else
#define main param_unused_params_example_B_main
#endif
#endif

extern "C"
int main(void) {
	// use only `a` and `d`:
	auto rv = a ? 7 : d * 10000;
	// thanks to type propagation, we should observe the `rv` type to have become `double`:
	static_assert(std::is_arithmetic<decltype(rv)>::value);
	static_assert(std::is_floating_point<decltype(rv)>::value);
	rv += f().x;

	auto& list = GlobalParams();
	auto lst = list.as_list();

	std::cout << std::format("Demo:\n\nWe expect to have calculated a value that's very close to 31418: {}\n\n", rv);
	std::cout << "\nWe also expect to see only 2 variables listed:\n";
	for (auto el : lst) {
		std::cout << std::format("var name: {}\n", el->name_str());
	}
	std::cout << std::format("\n--> parameter count = {}\n", lst.size());

	return 0;
}
