/*============================================================================
  
  pmbasic

  pmbasic.c

  Implementation of the PMBASIC command line. This module handles commands
  like NEW, LIST... but the real guts of the system are in 
  basicprogram.c and parser.c.

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

//#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "tokenizer.h"
#include "errcodes.h"
#include "parser.h"
#include "basicprogram.h"
#include "interface.h"
#include "strings.h"
#include "variabletable.h"
#include "tokenizer.h"

static char line [MAX_LINE];

#define MSG_BAD_LINE_NUMBER BASIC_ERR_BAD_LINE_NUMBER
#define MAX_ARGC 3 

typedef struct
  {
  int from;
  int count;
  int n;
  } ListIteratorData;

/*============================================================================
 * pmbasic_list_line_iterator
 * =========================================================================*/
#define LID ((ListIteratorData*) user_data)
static BOOL pmbasic_list_line_iterator (const BasicProgram *bp,
                 const char *b, const char *e, void *user_data)
  {
  (void)bp;
  VARTYPE line_num;
  if (basicprogram_get_line_number (b, &line_num))
    {
    if (line_num >= LID->from)
      {
      if  ( (LID->n < LID->count) || (LID->count == 0) )
        {
        int n = e - b;
        strncpy (line, b, MAX_LINE);
        line [n] = 0;
        interface_output_string (line);
        interface_output_endl();
        LID->n++;
        }
      }
    }

  return TRUE;
  }

/*============================================================================
 * pmbasic_parse_number
 * =========================================================================*/
VARTYPE pmbasic_parse_number (const char *s, uint8_t *conv)
  {
  // TODO -- I really need to unify all the number conversion functions
  //  that are scattered throughout this code.
  VARTYPE r;
  *conv = basicprogram_get_line_number (s, &r);
  return r;
  }

/*============================================================================
 * pmbasic_list
 * =========================================================================*/
static void pmbasic_list (const BasicProgram *bp, int argc, char **argv)
  {
  int from = 0;
  int count = 0;

  uint8_t conv;
  if (argc > 1)
    from = pmbasic_parse_number (argv[1], &conv);
  if (argc > 2)
    count = pmbasic_parse_number (argv[2], &conv);
  // TODO check conversion errors

  ListIteratorData ild;
  ild.from = from;
  ild.count = count;
  ild.n = 0;

  basicprogram_iterate_lines (bp, pmbasic_list_line_iterator, &ild);
  }

/*============================================================================
 * pmbasic_save
 * =========================================================================*/
static void pmbasic_save (const BasicProgram *bp, int argc, char **argv)
  {
  (void)argc;
  (void)argv;
  interface_save (bp);
  }

/*============================================================================
 * pmbasic_load
 * =========================================================================*/
static void pmbasic_load (BasicProgram *bp, int argc, char **argv)
  {
  (void)argc;
  (void)argv;
  interface_load (bp);
  }

/*============================================================================
 * pmbasic_info
 * =========================================================================*/
static void pmbasic_info (const BasicProgram *bp, int argc, char **argv)
  {
  (void)bp;
  (void)argc;
  (void)argv;
  
  strings_output_string (STRING_INDEX_VERSION);
  interface_output_endl ();
  strings_output_string (STRING_INDEX_PROG_SIZE);
  interface_output_number (basicprogram_get_length (bp));
  interface_output_string (" ");
  strings_output_string (STRING_INDEX_BYTES);
  interface_output_endl ();

  interface_info ();
  }

/*============================================================================
 * pmbasic_help
 * =========================================================================*/
static void pmbasic_help (const BasicProgram *bp, int argc, char **argv)
  {
  (void)bp;
  (void)argc;
  (void)argv;
  for (int i = 0; i < STRINGS_NUM_HELP; i++)
    {
    strings_get (STRINGS_FIRST_HELP + i, line, sizeof (line) - 1);
    interface_output_string (line);
    interface_output_endl();
    }
  }

/*============================================================================
 * pmbasic_run
 * =========================================================================*/
static void pmbasic_run (Parser* parser, const BasicProgram *bp, 
               int argc, char **argv)
  {
  (void)argc;
  (void)argv;
  if (parser_set_program (parser, bp))
    {
    parser_run (parser); // Reports its own erors
    }
  }

/*===========================================================================
  pmbasic_do_immediate
===========================================================================*/
static void pmbasic_do_immediate (BasicProgram *bp, Parser *parser, 
                         const char *line, BOOL *stop)
  {
  char iline2 [MAX_LINE];
  char *argv [MAX_ARGC];
  strncpy (iline2, line, MAX_LINE - 1);
  int argc = 0;
  char *tok = strtok (iline2, " \t");
  while (tok)
    {
    argv [argc] = tok;
    tok = strtok ((char *)0, " \t");
    argc++;
    }

  if (strings_compare_index (argv[0], STRING_INDEX_RUN))
    {
    pmbasic_run (parser, bp, argc, argv);
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_LIST))
    {
    pmbasic_list (bp, argc, argv);
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_QUIT))
    {
    *stop = TRUE;
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_SAVE))
    {
    pmbasic_save (bp, argc, argv);
    }
 else if (strings_compare_index (argv[0], STRING_INDEX_LOAD))
    {
    pmbasic_load (bp, argc, argv);
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_INFO))
    {
    pmbasic_info (bp, argc, argv);
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_NEW))
    {
    basicprogram_clear (bp);
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_HELP))
    {
    pmbasic_help (bp, argc, argv);
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_CLEAR))
    {
    parser_clear_variables (parser);
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_GOTO))
    {
    strings_output_string (BASIC_ERR_UNSUP_IMMEDIATE); 
    interface_output_endl();
    }
  else if (strings_compare_index (argv[0], STRING_INDEX_GOSUB))
    {
    strings_output_string (BASIC_ERR_UNSUP_IMMEDIATE); 
    interface_output_endl();
    }
  else
    {
    strncpy (iline2, line, MAX_LINE - 1);
    strcat (iline2, "\n");
    parser_run_line (parser, iline2);
    }

  }

/*===========================================================================
  pmbasic_process_line
===========================================================================*/
static void pmbasic_process_line (BasicProgram *bp, const char *line)
  {
  BasicProgramResult r = basicprogram_insert_line (bp, line);
  // I'm unsure exactly what responses need to be reported
  //   to the user.
  switch (r)
    {
    case BASICPROGRAM_UNCHANGED: break;

    case BASICPROGRAM_BAD_LINE_NUMBER:
      strings_output_string (MSG_BAD_LINE_NUMBER); 
      interface_output_endl();
      break;

    case BASICPROGRAM_LINE_DELETED:
      strings_output_string (STRING_INDEX_LINE_DELETED);
      interface_output_endl();
      break;

    case BASICPROGRAM_LINE_REPLACED:
    case BASICPROGRAM_LINE_APPENDED:
    case BASICPROGRAM_LINE_INSERTED:
      break;
    }
  }

/*===========================================================================
  pmbasic_main_loop
===========================================================================*/

void pmbasic_main_loop (void)
  {
  Parser *parser = parser_new();
  BasicProgram *bp = basicprogram_new_empty();
  VariableTable *vt = variabletable_new_empty();
  parser_set_variable_table (parser, vt);
  //basicprogram_set_program (bp, p);

  BOOL stop = FALSE;
  do
    {
    interface_output_string ("> ");
    uint8_t error = 0; // TODO
    interface_readstring (line, sizeof (line), &error);
    if (error)
      {
      strings_output_string (error);
      interface_output_endl();
      }
    else
      {
      if (line[0])
        {
        if (isalpha (line[0]) || line[0] == '?')
          pmbasic_do_immediate (bp, parser, line, &stop);
        else
          pmbasic_process_line (bp, line);
        }
      }
    } while (!stop);

  variabletable_destroy (vt);
  basicprogram_destroy (bp);
  parser_destroy (parser);
  }

