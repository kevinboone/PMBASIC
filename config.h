/*===========================================================================

  pmbasic

  config.h

  This header contains definitions of sizes and limits. These must
  be chosen carefully in the Arduino version, because RAM is so
  scarce.

===========================================================================*/
#pragma once

#include <stdint.h>

// Define whether to build a version with int or long arithmetic.
// This decision affects all data stored and managed by PMBASIC,
//   although it doesn't affect the program size particularly.
// Bear in mind that on AVRs, "long" is generally only 32-bit
//#define VARTYPE int16_t
#define VARTYPE int32_t 

// Define the decimal indicator supplied to the printf() function,
//   which depends on the integer size
#if VARIABLE_TYPE == long
#define PRINTF_DEC "%ld"
#else
#define PRINTF_DEC "%d"
#endif

// Longest number that can be parsed. This needs to be related to
//   the data type used to represent numbers in binary (e.g., int)
#if VARIABLE_TYPE == long
#define MAX_NUMBER 10
#else
#define MAX_NUMBER 5
#endif

// Longest hex number that can be parsed. This needs to be related to
//   the data type used to represent numbers in binary (e.g., int)
#if VARIABLE_TYPE == long
#define MAX_HEX_NUMBER 8
#else
#define MAX_HEX_NUMBER 4
#endif

// Longest line in total that we will handle. This includes the number,
// whitespace, and and terminating '\n', but not any terminating zero
#define MAX_LINE 81

// Largest number of nested GOSUB operations
#define MAX_GOSUB_STACK_DEPTH 10

// Largest character string that can be stored, in bytes (including
//  terminating zero)
#define MAX_STRINGLEN 41

// Largest depth of nested FOR loops
#define MAX_FOR_STACK_DEPTH 4

#define TOKEN_MAX_LENGTH 40




