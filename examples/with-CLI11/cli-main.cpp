
// showcase using libparameters in conjunction with the CLI11 command line parsing library.

#ifdef CLI11_SINGLE_FILE
#include "CLI11.hpp"
#else
#include "CLI/CLI.hpp"
#endif




#include <parameters/parameters.h>

#include "monolithic_examples.h"

#include "internal_helpers.hpp"

#include <iostream>
#include <format>

using namespace parameters;

namespace {

	BoolParam flag1(false, "f1", "bla bla bla bla", GlobalParams());
	IntParam flag2(999, "f2", "bla bla bla bla", GlobalParams());
	BoolParam flag3(false, "f3", "bla bla bla bla", GlobalParams());

	IntParam option1(42, "opt1", "bla bla bla bla", GlobalParams());

} // namespace 

#if defined(BUILD_MONOLITHIC)
#define main param_use_with_cli11_example_main
#endif

extern "C"
int main(int argc, const char **argv) {
	CLI::App app{"App description"};

	// Define options
	app.add_option("-o", option1, "Parameter");

	//bool flag_bool;
	app.add_flag("--bool,-b", flag1, "This is a bool flag");

	//int flag_int;
	app.add_flag("-i,--int", flag2, "This is an int flag");

	CLI::Option *flag_plain = app.add_flag("--plain,-p", "This is a plain flag");

	CLI11_PARSE(app, argc, argv);

	if (*flag_plain) {
		// cout << "Flag plain: " << flag_plain->count() << endl;
		flag3 = flag_plain->as<bool>();
	}

	std::cout << std::format("Parameter values:\n  option 1 = {}\n  flag 1 = {}\n  flag 2 = {}\n  flag 3 = {}\n", option1.value(), flag1.value(), flag2.value(), flag3.value());

	return 0;
}
