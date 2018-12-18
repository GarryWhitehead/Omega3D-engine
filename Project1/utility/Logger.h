#pragma once
#include <cstdio>
#include <stdio.h>

// simple file logging tools
#define LOGGER(format, ...) {					 \
	fprintf(stderr, format , __VA_ARGS__);		 \
	fflush(stderr);								 \
}

#define LOGGER_ERROR(...) {							 \
	fprintf(stderr, "{Error log}:", __VA_ARGS__);	 \
	fflush(stderr);									 \
}

#define LOGGER_INFO(...) {							 \
	fprintf(stderr, "{Info log}:", __VA_ARGS__);	 \
	fflush(stderr);									 \
}