//utility stuff

#ifndef LEMON_H_
#define LEMON_H_

#include <stdio.h>

//like assert but not disabled in release builds
#define strong_assert(expr_) do {\
	if (!(expr_)) {\
		fprintf(stderr, "\033[1mstrong_assert fail\033[0m: \033[31m`%s`\033[0m, at \033[4m%s:%i\033[0m\n", #expr_, __FILE__, __LINE__);\
		exit(1);\
	}\
} while (0)

#endif //LEMON_H_