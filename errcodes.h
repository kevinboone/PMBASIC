/*===========================================================================
  
  pmbasic

  errcodes.h

  List of error codes that are used by various modules
 
===========================================================================*/

#pragma once

// CRUCIAL! Error code numbers must match the order of strings in
//  the string table in strings.c.

#define BASIC_ERR_NONE                 0
#define BASIC_ERR_TOKEN_TOO_LONG       1
#define BASIC_ERR_TOKENIZER_INTERNAL   2
#define BASIC_ERR_NOMEM                3
#define BASIC_ERR_NO_LINE_NUM          4
#define BASIC_ERR_SYNTAX               5
#define BASIC_ERR_INPUT_TOO_LONG       6
#define BASIC_ERR_INTERRUPTED          7
#define BASIC_ERR_BAD_LINE_NUMBER      8
#define BASIC_ERR_DIV_ZERO             9
#define BASIC_ERR_UNDEFINED_VAR        10
#define BASIC_ERR_UNKNOWN_LINE         11
#define BASIC_ERR_GOSUB_DEPTH          12
#define BASIC_ERR_RETURN_WITHOUT_GOSUB 13
#define BASIC_ERR_FOR_DEPTH            14
#define BASIC_ERR_NEXT_WITHOUT_FOR     15
#define BASIC_ERR_NUMBER_TOO_LONG      16
#define BASIC_ERR_MALFORMED_NUMBER     17
#define BASIC_ERR_UNSUP_IMMEDIATE      18
#define BASIC_ERR_UNEXPECTED_TOKEN     19
#define BASIC_ERR_UNPRINTABLE_TOKEN    20 
#define BASIC_ERR_NO_FOR_VAR           21 
#define BASIC_ERR_NO_FOR_EQ            22 
#define BASIC_ERR_NO_FOR_TO            23 
#define BASIC_ERR_VAR_NO_EQ            24 
#define BASIC_ERR_KW_NO_VAR            25 
#define BASIC_ERR_EXPECTED_COMMA       26
#define BASIC_ERR_NO_STORED_PROGRAM    27
#define BASIC_ERR_PROGRAM_TOO_LARGE    28



