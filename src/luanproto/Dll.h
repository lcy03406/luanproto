#pragma once
#ifdef _WIN32
#ifdef LIBLUANP_EXPORTS
#define LIBLUANP_API __declspec(dllexport)
#else
#define LIBLUANP_API __declspec(dllimport)
#endif
#else
#define LIBLUANP_API 
#endif
