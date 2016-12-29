#ifndef CHIROBAT_DEBUG
#define CHRIOBAT_DEBUG

#include <cstdio>
#include <cstdlib>

// register function pointers to an error bank for automated error reporting
// for now, a manual macro set

#if( defined DEBUG || defined _DEBUG)

#define LOG_ERR(string, ...) fprintf(stderr, string, ##__VA_ARGS__); fprintf(stderr, "\n")

#define RET_ON_ERR(funcRet, errRet, string, ...) \
do {\
if (funcRet)\
{\
	LOG_ERR(string, ##__VA_ARGS__);\
	return errRet;\
}\
}while (0)

#define RET_ON_NEG_ERR(funcRet, errRet, string, ...) \
do {\
if (funcRet < 0)\
{\
	LOG_ERR(string, ##__VA_ARGS__);\
	return errRet;\
}\
}while (0)

#else

#define LOG_ERR(string, ...)

#define RET_ON_ERR(funcRet, errRet, string, ...) \
do {\
if (funcRet)\
	return errRet;\
}while (0)

#define RET_ON_NEG_ERR(funcRet, errRet, string, ...) \
do {\
if (funcRet < 0)\
	return errRet;\
}while (0)

#endif

#endif
