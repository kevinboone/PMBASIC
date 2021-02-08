/*============================================================================
  
  pmbasic

  strings.c

  stringtable and string handling functions. Note that we need a 
  centralized string table in the Arduino interface, so that strings
  can be managed in flash, and only loaded into RAM as required.

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#include <string.h>
#include <ctype.h>
#ifdef ARDUINO
// For PROGMEM
#include <avr/pgmspace.h>
#endif 

#include "strings.h"
#include "config.h"
#include "interface.h"

#ifndef ARDUINO
#define PROGMEM
#endif

const char STRING_DUMMY[] PROGMEM = "";

// Crucial!! These PROGMEM constants can only be accessed through
//   strings_get. They are not real addresses in memory, but must
//   be copied in a specific way from flash.
const char STRING_OK[] PROGMEM = "OK";
const char ERRMSG_ERR_TOKEN_TOO_LONG[] PROGMEM = "Token too long";
const char ERRMSG_ERR_TOKENIZER_INTERNAL[] PROGMEM = "Tokenizer internal error";
const char ERRMSG_ERR_NOMEM[] PROGMEM = "Out of memory";
const char ERRMSG_ERR_NO_LINE_NUM[] PROGMEM = "Unnumbered line";
const char ERRMSG_ERR_SYNTAX[] PROGMEM = "Syntax error";
const char ERRMSG_ERR_INPUT_TOO_LONG[] PROGMEM = "Input too long";
const char ERRMSG_ERR_INTERRUPTED[] PROGMEM = "Interrupted";
const char ERRMSG_ERR_BAD_LINE_NUMBER[] PROGMEM = "Bad line number";
const char ERRMSG_ERR_DIV_ZERO[] PROGMEM = "Division by zero";
const char ERRMSG_ERR_UNDEFINED_VARIABLE[] PROGMEM = "Undefined variable";
const char ERRMSG_ERR_UNKNOWN_LINE[] PROGMEM = "Undefined variable";
const char ERRMSG_ERR_GOSUB_STACK_DEPTH[] PROGMEM = "Too many nested GOSUBs";
const char ERRMSG_ERR_RET_GOSUB[] PROGMEM = "RETURN without GOSUB";
const char ERRMSG_ERR_FOR_STACK_DEPTH[] PROGMEM = "Too many nested FORs";
const char ERRMSG_ERR_NEXT_WITHOUT_FOR[] PROGMEM = "NEXT without FOR";
const char ERRMSG_ERR_NUMBER_TOO_LONG[] PROGMEM = "Number too long";
const char ERRMSG_ERR_MALFORMED_NUMBER[] PROGMEM = "Malformed number";
const char ERRMSG_ERR_UNSUP_IMMEDIATE[] PROGMEM 
        = "Unsupported immediate statement";
const char ERRMSG_ERR_UNEXPECTED_TOKEN[] PROGMEM = "Unexpected token";
const char ERRMSG_ERR_UNPRINTABLE_TOKEN[] PROGMEM = "Unprintable token";
const char ERRMSG_ERR_NO_FOR_VAR[] PROGMEM = "FOR must be followed by a variable";
const char ERRMSG_ERR_NO_FOR_EQ[] PROGMEM = "FOR without '=' sign";
const char ERRMSG_ERR_NO_FOR_TO[] PROGMEM = "FOR without TO"; 
const char ERRMSG_ERR_VAR_NO_EQ[] PROGMEM = "Expected '=' after name"; 
const char ERRMSG_ERR_KW_NO_VAR[] PROGMEM = "Expected variable after keyword"; 
const char ERRMSG_ERR_EXPECTED_COMMA[] PROGMEM = "Expected comma";
const char ERRMSG_ERR_NO_STORED_PROGRAM[] PROGMEM = "No stored program";
const char ERRMSG_ERR_PROGRAM_TOO_LARGE[] PROGMEM = "Expected comma";

const char STRING_PRINT[] PROGMEM = "print";
const char STRING_IF[] PROGMEM = "if";
const char STRING_THEN[] PROGMEM = "then";
const char STRING_ELSE[] PROGMEM = "else";
const char STRING_NOT[] PROGMEM = "not";
const char STRING_GOTO[] PROGMEM = "goto";
const char STRING_GOSUB[] PROGMEM = "gosub";
const char STRING_END[] PROGMEM = "end";
const char STRING_RETURN[] PROGMEM = "return";
const char STRING_REM[] PROGMEM = "rem";
const char STRING_FOR[] PROGMEM = "for";
const char STRING_NEXT[] PROGMEM = "next";
const char STRING_TO[] PROGMEM = "to";
const char STRING_LET[] PROGMEM = "let";
const char STRING_INPUT[] PROGMEM = "input";
const char STRING_MILLIS[] PROGMEM = "millis";
const char STRING_DELAY[] PROGMEM = "delay";
const char STRING_PEEK[] PROGMEM = "peek";
const char STRING_POKE[] PROGMEM = "poke";
const char STRING_DIGITALREAD[] PROGMEM = "digitalread";
const char STRING_DIGITALWRITE[] PROGMEM = "digitalwrite";
const char STRING_PINMODE[] PROGMEM = "pinmode";
const char STRING_ANALOGREAD[] PROGMEM = "analogread";
const char STRING_ANALOGWRITE[] PROGMEM = "analogwrite";

const char STRING_GEN_LINE_DELETED[] PROGMEM = "Line deleted";
const char STRING_GEN_PROG_SIZE[] PROGMEM = "Program size: "; 
const char STRING_GEN_BYTES[] PROGMEM = "bytes"; 
const char STRING_GEN_TOT_RAM[] PROGMEM = "Total RAM: "; 
const char STRING_GEN_TOT_EEPROM[] PROGMEM = "Total EEPROM: "; 
const char STRING_GEN_VERSION[] PROGMEM = "PMBASIC version 0.1"; 
const char STRING_GEN_FREE_RAM[] PROGMEM = "Free RAM: "; 

const char STRING_CMD_LIST[] PROGMEM = "list";
const char STRING_CMD_RUN[] PROGMEM = "run";
const char STRING_CMD_QUIT[] PROGMEM = "quit";
const char STRING_CMD_SAVE[] PROGMEM = "save";
const char STRING_CMD_LOAD[] PROGMEM = "load";
const char STRING_CMD_INFO[] PROGMEM = "info";
const char STRING_CMD_NEW[] PROGMEM = "new";
const char STRING_CMD_HELP[] PROGMEM = "help";
const char STRING_CMD_CLEAR[] PROGMEM = "clear";

const char STRING_H1[] PROGMEM = "Lines beginning with a number are stored as program lines.";
const char STRING_H2[] PROGMEM = "New lines replace existing lines with the same number.";
const char STRING_H3[] PROGMEM = "Entering a number on its own deletes an existing line.";
const char STRING_H4[] PROGMEM = "Unnumbered lines are treated as commands or BASIC statements,";
const char STRING_H5[] PROGMEM = "and executed immediately.";
const char STRING_H6[] PROGMEM = "Commands:";
const char STRING_H7[] PROGMEM = "  NEW : clear the existing program";
const char STRING_H8[] PROGMEM = "  LIST [start] [count] : list the program";
const char STRING_H9[] PROGMEM = "  SAVE : save the program to EEPROM";
const char STRING_H10[] PROGMEM = "  LOAD : load a program from EEPROM";
const char STRING_H11[] PROGMEM = "  INFO : show memory sizes, etc";
const char STRING_H12[] PROGMEM = "  CLEAR : clear global variables";

const char *const strings[] PROGMEM =
  {
  STRING_OK,
  ERRMSG_ERR_TOKEN_TOO_LONG,
  ERRMSG_ERR_TOKENIZER_INTERNAL,
  ERRMSG_ERR_NOMEM,
  ERRMSG_ERR_NO_LINE_NUM,
  ERRMSG_ERR_SYNTAX,
  ERRMSG_ERR_INPUT_TOO_LONG,
  ERRMSG_ERR_INTERRUPTED,
  ERRMSG_ERR_BAD_LINE_NUMBER,
  ERRMSG_ERR_DIV_ZERO,
  ERRMSG_ERR_UNDEFINED_VARIABLE,
  ERRMSG_ERR_UNKNOWN_LINE,
  ERRMSG_ERR_GOSUB_STACK_DEPTH,
  ERRMSG_ERR_RET_GOSUB,
  ERRMSG_ERR_FOR_STACK_DEPTH,
  ERRMSG_ERR_NEXT_WITHOUT_FOR,
  ERRMSG_ERR_NUMBER_TOO_LONG,
  ERRMSG_ERR_MALFORMED_NUMBER,
  ERRMSG_ERR_UNSUP_IMMEDIATE,
  ERRMSG_ERR_UNEXPECTED_TOKEN,
  ERRMSG_ERR_UNPRINTABLE_TOKEN,
  ERRMSG_ERR_NO_FOR_VAR,
  ERRMSG_ERR_NO_FOR_EQ,
  ERRMSG_ERR_NO_FOR_TO,
  ERRMSG_ERR_VAR_NO_EQ,
  ERRMSG_ERR_KW_NO_VAR,
  ERRMSG_ERR_EXPECTED_COMMA,
  ERRMSG_ERR_NO_STORED_PROGRAM,
  ERRMSG_ERR_PROGRAM_TOO_LARGE,
  STRING_DUMMY,
  STRING_PRINT,
  STRING_IF,
  STRING_THEN,
  STRING_ELSE,
  STRING_NOT,
  STRING_GOTO,
  STRING_GOSUB,
  STRING_END,
  STRING_RETURN,
  STRING_REM,
  STRING_FOR,
  STRING_NEXT,
  STRING_TO,
  STRING_LET,
  STRING_INPUT,
  STRING_MILLIS,
  STRING_DELAY,
  STRING_PEEK,
  STRING_POKE,
  STRING_DIGITALREAD,
  STRING_DIGITALWRITE,
  STRING_PINMODE,
  STRING_ANALOGREAD,
  STRING_ANALOGWRITE,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_GEN_LINE_DELETED,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_GEN_PROG_SIZE,
  STRING_GEN_BYTES,
  STRING_GEN_TOT_RAM,
  STRING_GEN_TOT_EEPROM,
  STRING_GEN_VERSION,
  STRING_GEN_FREE_RAM,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_CMD_LIST,
  STRING_CMD_RUN,
  STRING_CMD_QUIT,
  STRING_CMD_SAVE,
  STRING_CMD_LOAD,
  STRING_CMD_INFO,
  STRING_CMD_NEW,
  STRING_CMD_HELP,
  STRING_CMD_CLEAR,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_DUMMY,
  STRING_H1,
  STRING_H2,
  STRING_H3,
  STRING_H4,
  STRING_H5,
  STRING_H6,
  STRING_H7,
  STRING_H8,
  STRING_H9,
  STRING_H10,
  STRING_H11,
  STRING_H12,
  };

/*===========================================================================
  strings_get
===========================================================================*/
void strings_get (uint8_t n, char *buff, uint8_t len)
  {
#ifdef ARDUINO
  strncpy_P (buff, (char *)pgm_read_word (&strings[n]), len - 1);
#else
  strncpy (buff, strings[n], len - 1);
#endif
  }

/*===========================================================================
  strings_compare_index
===========================================================================*/
BOOL strings_compare_index (const char *s, uint8_t index)
  {
  char buff [TOKEN_MAX_LENGTH + 1];
  strings_get (index, buff, TOKEN_MAX_LENGTH);
  int i = 0;
  while (s[i] && buff[i])
    {
    if (tolower (s[i]) != buff[i]) return FALSE; 
    i++;
    }
  if (s[i] && !buff[i]) return FALSE;
  if (!s[i] && buff[i]) return FALSE;
  return TRUE;
  }

/*===========================================================================
  strings_output_string
===========================================================================*/
void strings_output_string (uint8_t index)
  {
  char buff [TOKEN_MAX_LENGTH + 1];
  strings_get (index, buff, TOKEN_MAX_LENGTH);
  interface_output_string (buff);
  }



