
// showcase the scenario using libparameters in conjunction with the classic getopt / getopt_long API.

#include <getopt.h>


#include <parameters/parameters.h>

#include "internal_helpers.hpp"

#include <iostream>
#include <format>
#include <cassert>

using namespace parameters;

namespace {

	BoolParam flag1(false, "f1", "bla bla bla bla", GlobalParams());
	IntParam flag2(999, "f2", "bla bla bla bla", GlobalParams());
	StringParam flag3("bugger", "f3", "bla bla bla bla", GlobalParams());

#if 0
	struct option opts[] = {
	  {"first", no_argument, 0, 'a'},
	  {"second", required_argument, 0, 'b'},
	  {"third", optional_argument, 0, 'c'},
	  {0, 0, 0, 0}
	};
#endif

} // namespace 

#if defined(BUILD_MONOLITHIC)
#define main param_use_with_getopt_example_main
#endif

extern "C"
int main(int argc, const char **argv) {
	for (;;) {
		auto opt = getopt(argc, argv, "ab:c::");
		switch (opt) {
		case -1:
			break;

		case 'a':
			flag1 = true;
			continue;
		case 'b':
			flag2.set_value(optarg, PARAM_VALUE_IS_SET_BY_COMMANDLINE);
			continue;
		case 'c':
			if (optarg != nullptr) {
				flag3 = optarg;
			} else {
				flag3 = "default";
			}
			continue;
		case '?':
			std::cout << "Help info: run with command line arguments -a, -b <NUMBER> or -c [<TEXT>]\n";
			break;
		default:
			assert(0);
			break;
		}
		break;
	}
	std::cout << std::format("\n\nParameter values:\n\n  a = {}\n  b = {}\n  c = \"{}\"\n\n", flag1.value(), flag2.value(), flag3.value());
	return 0;
}
