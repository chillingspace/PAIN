#pragma once

#ifdef PN_PLATFORM_WINDOWS
	#ifdef PN_BUILD_DLL
		#define PAIN_API __declspec(dllexport)
	#else
		#define PAIN_API __declspec(dllimport)
	#endif // PN_BUILD_DLL
#else
	#error PAINEngine only supports windows

#endif // PN_PLATFORM_WINDOWS
