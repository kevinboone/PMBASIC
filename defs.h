#pragma once

#include <stdint.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE 
#define FALSE 0
#endif

#ifdef __cplusplus
#define BEGIN_DECLS extern "C" { 
#define END_DECLS }
#else
#define BEGIN_DECLS 
#define END_DECLS 
#endif

#ifndef BOOL
typedef uint8_t BOOL;
#endif

