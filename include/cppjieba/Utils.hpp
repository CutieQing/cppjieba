#pragma once

#include <filesystem>
#if defined(__APPLE__) && defined(__MACH__)
    /* Apple OSX and iOS (Darwin). */
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE == 1
    /* iOS */
inline std::string get_temp_dir(){
	return getenv("HOME") + "/tmp"
}
#endif
#else
inline std::string get_temp_dir(){
	return "/tmp";
}
#endif
