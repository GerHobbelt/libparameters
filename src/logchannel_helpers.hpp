
#pragma once

#if !defined(BUILD_MONOLITHIC)

#include <fmt/format.h>       // for fmt

#define PARAM_ERROR(...)		fmt::print(__VA_ARGS__)
#define PARAM_WARN(...)		fmt::print(__VA_ARGS__)
#define PARAM_INFO(...)		fmt::print(__VA_ARGS__)
#define PARAM_DEBUG(...)		fmt::print(__VA_ARGS__)
#define PARAM_TRACE(...)		fmt::print(__VA_ARGS__)

#else

#include <tesseract/tprintf.h>

#define PARAM_ERROR(...)		tesseract::tprintError(__VA_ARGS__)
#define PARAM_WARN(...)		tesseract::tprintWarn(__VA_ARGS__)
#define PARAM_INFO(...)		tesseract::tprintInfo(__VA_ARGS__)
#define PARAM_DEBUG(...)		tesseract::tprintDebug(__VA_ARGS__)
#define PARAM_TRACE(...)		tesseract::tprintTrace(__VA_ARGS__)

#endif

