
#include <parameters/parameters.h>

#include <cstdio>

#include <gtest/gtest.h>

#if defined(BUILD_MONOLITHIC)
#define main	parameters_test_main
#endif

int main(int argc, const char** argv) {
	printf("Running main() from %s\n", __FILE__);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
