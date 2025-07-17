
#pragma once

#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"

// -----------------------------------------------------------------------

#include "./ParamArrayType.cpp"
#include "./ParamArrayType_NumericBaseType.cpp"
#include "./ParamArrayType_StringBaseType.cpp"
#include "./ParamArrayType_UserDefinedClassBaseType.cpp"

#include "./ParamBaseType.cpp"
#include "./ParamCoreType_Boolean.cpp"
#include "./ParamCoreType_FloatingPoint.cpp"
#include "./ParamCoreType_Integer.cpp"
#include "./ParamCoreType_String.cpp"
#include "./ParamCoreType_UserDefinedClass.cpp"

#include "./paramsAssist.cpp"
#include "./ParamsVector.cpp"
#include "./ParamsVectorSet.cpp"
#include "./ReadWriteConfigFile.cpp"
#include "./ReportFile.cpp"
#include "./SetApplicationName.cpp"
#include "./Snapshots.cpp"
#include "./Utilities.cpp"
#include "./ConfigFile.cpp"
#include "./CString.cpp"
#include "./empty.cpp"
#include "./FindParam.cpp"
#include "./globals.cpp"
#include "./Reporting.cpp"
#include "./ReportWriters.cpp"
#include "./ResetToDefaults.cpp"
#include "./PrintParams.cpp"
