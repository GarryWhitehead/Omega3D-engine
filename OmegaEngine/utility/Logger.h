#pragma once

#include <cstdio>
#include <stdio.h>

// disable warning about passing too many args as format string
#pragma warning(disable : 4474)

// simple file logging tools
#define LOGGER(format, ...)                   \
	do                                        \
	{                                         \
		fprintf(stderr, format, __VA_ARGS__); \
		fprintf(stderr, "\n");                \
		fflush(stderr);                       \
	} while (false)

#define LOGGER_ERROR(...)                             \
	do                                                \
	{                                                 \
		fprintf(stderr, "{Error log}: " __VA_ARGS__); \
		fprintf(stderr, "\n");                        \
		fflush(stderr);                               \
	} while (false)

#define LOGGER_INFO(...)                             \
	do                                               \
	{                                                \
		fprintf(stderr, "{Info log}: " __VA_ARGS__); \
		fprintf(stderr, "\n");                       \
		fflush(stderr);                              \
	} while (false)

#define LOGGER_WARN(...)                                \
	do                                                  \
	{                                                   \
		fprintf(stderr, "{Warning log}: " __VA_ARGS__); \
		fprintf(stderr, "\n");                          \
		fflush(stderr);                                 \
	} while (false)
