#pragma once
#ifdef _WIN32
#ifdef LIBBATTLE_EXPORTS
#define LIBBATTLE_API __declspec(dllexport)
#else
#define LIBBATTLE_API __declspec(dllimport)
#endif
#else
#define LIBBATTLE_API 
#endif
