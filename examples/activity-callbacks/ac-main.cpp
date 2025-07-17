
// showcases the scenario where we use advanced features of the libparameters library including userland defined activity callbacks providing custom processing of parameter activities & statistics.

#include <parameters/parameters.h>

#include <iostream>
#include <format>
#include <cassert>

using namespace parameters;

static ParamsVector &ParamsManager(void) {
	static ParamsVector global_params("activity-callbacks"); // static auto-inits at startup
	return global_params;
}


#if defined(BUILD_MONOLITHIC)
#define main param_activity_callback_example_main
#endif

extern "C"
int main(void) {
	BoolParam::TheEventHandlers evt_handlers_4_bool{
		.on_modify_f = [](BoolParam &target, const bool old_value, bool &new_value, const bool default_value, ParamSetBySourceType source_type, ParamPtr optional_setter) {
			std::cout << std::format("\nevent: modifying parameter: {}\n", target.name_str());
			new_value = old_value;
		},
		.on_validate_f = [](BoolParam &target, const bool old_value, bool &new_value, const bool default_value, ParamSetBySourceType source_type) {
			std::cout << std::format("\nevent: validating new value for parameter: {} :: old:{} --> new:{} / default:{}\n", target.name_str(), old_value, new_value, default_value);
			new_value = old_value;
		},
		.on_parse_f = [](BoolParam& target, bool& new_value, const std::string &source_value_str, unsigned int &pos, ParamSetBySourceType source_type) {
			std::cout << std::format("\nevent: parsing input string for parameter: {} :: {}\n", target.name_str(), source_value_str);
		},
		.on_format_f = [](BoolParam &source, const bool value, const bool default_value, ValueFetchPurpose purpose) -> std::string {
			std::cout << std::format("\nevent: formatting parameter to string: {} = {}\n", source.name_str(), value);
			return std::format("custom: {}", value);
		},
	};

	BoolParam flag1(false, "f1", "bla bla bla bla", ParamsManager(), evt_handlers_4_bool);
	IntParam flag2(999, "f2", "bla bla bla bla", ParamsManager(), {});
	StringParam flag3("bugger", "f3", "bla bla bla bla", ParamsManager(), {});

	flag1 = true;
	flag2.set_value("1234");
	flag3 = "default";

	std::cout << std::format("\n\nParameter values:\n\n  flag1 = {}\n  flag2 = {}\n  flag3 = \"{}\"\n\n", flag1.value(), flag2.value(), flag3.value());
	return 0;
}

