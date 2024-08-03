
#include <parameters/parameters.h>

#include "internal_helpers.hpp"
#include "logchannel_helpers.hpp"
#include "os_platform_helpers.hpp"


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

}	// namespace
