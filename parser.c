/*===========================================================================

  pmbasic

  parser.c

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "config.h"
#include "defs.h"
#include "config.h"
#include "tokenizer.h"
#include "parser.h"
#include "klist.h"
#include "basicprogram.h"
#include "strings.h"
#include "interface.h"
#include "variabletable.h"
#include "errcodes.h"

/*===========================================================================
  Parser 
===========================================================================*/

typedef struct ForState
  {
  const char *back_pos;
  char *var_name;
  VARTYPE to;
  } ForState;

struct _Parser
  {
  const BasicProgram *bp; 

  // Index mapping line numbers to offsets in program
  KList *line_index;

  VARTYPE current_line;
  VariableTable *vt;

  // Subroutine stack and its depth
  // Note that we store the offset into the program text, not the line no. 
  const char *gosub_stack [MAX_GOSUB_STACK_DEPTH];
  uint8_t gosub_stack_ptr;

  // FOR state and its depth
  ForState for_stack [MAX_FOR_STACK_DEPTH];
  uint8_t for_stack_ptr;

  // ended is set when END is parsed
  BOOL ended;
  };

typedef struct _LineIndexEntry 
  {
  VARTYPE n;
  const char *start;
  } LineIndexEntry;

static VARTYPE parser_branch_factor (Parser *self, 
         Tokenizer *t, uint8_t *error); //FWD
static void parser_branch_statement (Parser *self, 
         Tokenizer *t, uint8_t *error); // FWD

/*===========================================================================
  parser_new
===========================================================================*/
Parser *parser_new (void)
  {
  Parser *self = malloc (sizeof (Parser));
  if (self)
    {
    self->line_index = NULL;
    }
  return self;
  }

/*===========================================================================
  parser_clear_index
===========================================================================*/
static void parser_clear_line_index (Parser *self)
  {
  if (self->line_index)
    {
    klist_destroy (self->line_index);
    }
  self->line_index = NULL;
  }

/*===========================================================================
  parser_destroy
===========================================================================*/
void parser_destroy (Parser *self)
  {
  parser_clear_line_index (self);
  free (self);
  }

/*===========================================================================
  parser_iterate_lines_for_index
===========================================================================*/
typedef struct 
  {
  Parser *self;
  uint8_t error;
  } ILI;

static BOOL parser_iterate_lines_for_index (const BasicProgram *self, 
                 const char *b, const char *e, void *user_data)
  {
  (void)self;
  (void)e;
  BOOL ret = FALSE;
  VARTYPE n;
  BOOL gotnum = basicprogram_get_line_number (b, &n);
  if (gotnum)
    {
    LineIndexEntry *lie = malloc (sizeof (LineIndexEntry));
    if (lie)
      {
      lie->n = n;
      lie->start = b;
      klist_append (((ILI *)user_data)->self->line_index, lie);
      ret = TRUE;
      }
    else
      ((ILI *)user_data)->error = BASIC_ERR_NOMEM; 
    }
  else
    {
    ((ILI *)user_data)->error = BASIC_ERR_NO_LINE_NUM; 
    }
  return ret;
  }

/*===========================================================================
  parser_dump_line_index
===========================================================================*/
void parser_dump_line_index (const Parser *self)
  {
  int l = klist_length (self->line_index);
  for (int i = 0; i < l; i++)
    {
    const LineIndexEntry *lie = klist_get (self->line_index, i);
    printf ("n=" PRINTF_DEC " pos=%s\n", lie->n, lie->start);
    }
  }

/*===========================================================================
  parser_index_lines
===========================================================================*/
static BOOL parser_index_lines (Parser *self, 
              uint8_t *err_code)
  {
  BOOL ret = FALSE;

  if (self->line_index)
    parser_clear_line_index (self);

  self->line_index = klist_new_empty (free);
  if (self->line_index)
    {
    ILI ili;
    ili.error = 0;
    ili.self = self;
    basicprogram_iterate_lines (self->bp, 
       parser_iterate_lines_for_index, &ili);

    if (ili.error == 0)
      ret = TRUE;
    else
      *err_code = ili.error;
    }
  else
    *err_code = BASIC_ERR_NOMEM;

  //parser_dump_line_index (self);

  return ret;
  }

/*===========================================================================
  parser_emit_if_error
===========================================================================*/
void parser_emit_if_error (const Parser *self, 
                             const Tokenizer *t, uint8_t error)
  {
  if (error)
    {
    strings_output_string (error);
    interface_output_string (", line: ");
    interface_output_number (self->current_line);
    const char *word = tokenizer_get_word (t);
    if (word[0])
      {
      interface_output_string (" near: ");
      interface_output_string (word);
      }
    interface_output_endl();
    }
  }

/*===========================================================================
  parser_set_program
===========================================================================*/
BOOL parser_set_program (Parser *self, const BasicProgram *bp)
  {
  static char buff[30];
  self->bp = bp;
  self->current_line = 0;
  uint8_t err_code = 0;
  parser_index_lines (self, &err_code);
  if (err_code)
    {
    strings_get (err_code + STRINGS_FIRST_ERR_CODE, buff, MAX_LINE);  
    interface_output_string (buff);
    interface_output_endl();
    }
  return (err_code == 0);
  }

/*===========================================================================
  parser_accept_symbol
===========================================================================*/
void parser_accept_symbol (Parser* self, Tokenizer *t, 
       char sym, uint8_t *error)
  {
  (void)self;
  if (tokenizer_is_symbol (t, sym))
    tokenizer_next (t, error);
  else
    *error = BASIC_ERR_UNEXPECTED_TOKEN;
  }

/*===========================================================================
  parser_branch_expr
===========================================================================*/
static VARTYPE parser_branch_term (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  VARTYPE t1 = parser_branch_factor (self, t, error); 
  if (*error) return 0;

  char op = tokenizer_get_sym (t);
  if (*error) return 0;

  while (op == '*' || op == '/' || op == '%') 
    {
    tokenizer_next (t, error);
    VARTYPE t2 = parser_branch_factor (self, t, error);
    if (*error) return 0;
    switch (op)
      {
      case '*': t1 *= t2; break; 
      case '/': 
        if (t2 == 0)
          {
          *error = BASIC_ERR_DIV_ZERO;
          return 0;
          }
        t1 /= t2;
        break;
      case '%': 
        if (t2 == 0)
          {
          *error = BASIC_ERR_DIV_ZERO;
          return 0;
          }
        t1 %= t2;
        break;
      }
    op = tokenizer_get_sym (t);
    }
  return t1;
  }

/*===========================================================================
  parser_branch_expr
===========================================================================*/
static VARTYPE parser_branch_expr (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word (t);
    if (strings_compare_index (word, STRING_INDEX_NOT))
      {
      tokenizer_next (t, error);
      return !parser_branch_expr (self, t, error); 
      }
    }

  VARTYPE t1 = parser_branch_term (self, t, error); 
  char op = tokenizer_get_sym (t);
  while (op == '+' || op == '-' || op == '&' || op == '|' || op == '<' 
          || op == '>' || op == '=') 
    {
    tokenizer_next (t, error);
    VARTYPE t2 = parser_branch_term (self, t, error);
    switch (op)
       {
       case '+': t1 += t2; break;
       case '-': t1 -= t2; break;
       case '&': t1 &= t2; break;
       case '<': t1 = (t1 < t2); break;
       case '>': t1 = (t1 > t2); break;
       case '=': t1 = (t1 == t2); break;
       case '|': t1 |= t2; break;
       } 
    op = tokenizer_get_sym (t);
    }
  return t1;
  }

/*===========================================================================
  parser_branch_factor
===========================================================================*/
static VARTYPE parser_branch_factor (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  if (tokenizer_is_number (t))
    {
    VARTYPE r = tokenizer_get_number_value (t);
    tokenizer_next (t, error);
    return r;
    }
  else if (tokenizer_is_symbol (t, '-'))
    {
    tokenizer_next (t, error);
    return -parser_branch_factor (self, t, error);
    }
  else if (tokenizer_is_symbol (t, '('))
    {
    tokenizer_next (t, error);
    if (*error) return 0;
    VARTYPE r = parser_branch_expr (self, t, error);
    parser_accept_symbol (self, t, ')', error);
    if (*error) return 0;
    return r;
    }
  else if (tokenizer_is_word (t))
    {
    VARTYPE r = 0;
    if (!variabletable_get_number (self->vt, tokenizer_get_word (t), &r))
      {
      strings_output_string (BASIC_ERR_UNDEFINED_VAR);
      interface_output_string (": ");
      interface_output_string (": ");
      interface_output_string (tokenizer_get_word (t));
      interface_output_endl ();
      *error = BASIC_ERR_UNDEFINED_VAR; 
      }
    tokenizer_next (t, error);
    return r;
    } 
  else
    {
    *error = BASIC_ERR_SYNTAX;
    return 0;;
    }
  }

/*===========================================================================
  parser_skip_to_next_line
===========================================================================*/
static void parser_skip_to_next_line (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  (void)self;
  do
    {
    tokenizer_next (t, error);
    } while (!tokenizer_is_eol (t)); 
  // Don't skip the \n -- the main parser loop always advances one
  //  token after the numbered-statement branch. This skips the \n, so
  //  if we skip here as well, we'll skip the line number
  //tokenizer_next (t, error);
  }

/*===========================================================================
  parser_branch_print_statement
===========================================================================*/
static void parser_branch_print_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error);
  if (*error) return;

  if (tokenizer_is_eol (t)) 
    {
    interface_output_endl();
    return;
    }

  BOOL no_newline = FALSE;
 
  // PRINT prints all its arguments, and doesn't know in advance
  //  how many. So what if it comes across ELSE? Most keywords will
  //  just cause a stop. But ELSE is legitimate in situations like
  //  IF foo THEN print 1 2 3 ELSE... So, lacking any better ideas,
  //  the program just skips to the end of line on an ELSE.
  do
    {
    if (tokenizer_is_string (t))
      {
      const char *string = tokenizer_get_string (t);
      interface_output_string (string);
      tokenizer_next (t, error);
      }
    else if (tokenizer_is_symbol (t, ','))
      {
      interface_output_string (" ");
      tokenizer_next (t, error);
      }
    else if (tokenizer_is_symbol (t, ';'))
      {
      no_newline = TRUE;
      tokenizer_next (t, error);
      }
    else if (tokenizer_is_symbol (t, '(') ||
             tokenizer_is_symbol (t, '-') ||
             tokenizer_is_number (t)) 
      {
      VARTYPE r = parser_branch_expr (self, t, error); 
      if (!*error)
        interface_output_number (r);  
      else
        return;
      }
    else if (tokenizer_is_word (t))
      {
      const char *word = tokenizer_get_word (t);
      if (strings_compare_index (word, STRING_INDEX_ELSE))
        {
        parser_skip_to_next_line (self, t, error);
        if (*error) return;
        }
      else
        {
        VARTYPE r = parser_branch_expr (self, t, error); 
        if (!*error)
          interface_output_number (r);  
        else
          return;
        }
      }
    else 
      {
      *error = BASIC_ERR_UNPRINTABLE_TOKEN;
      strings_output_string (BASIC_ERR_UNPRINTABLE_TOKEN); 
      const char *word = tokenizer_get_word (t);
      if (word[0])
        {
        interface_output_string (": ");
        interface_output_string (word);
        }
      interface_output_endl ();
      return;
      }
    } while (!tokenizer_is_eol (t)); 

  if (!no_newline)
    interface_output_endl();
  }

/*===========================================================================
  parser_branch_goto_statement
===========================================================================*/
static void parser_branch_goto_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Slurp GOTO 
  VARTYPE l = parser_branch_expr (self, t, error);
  if (!*error)
    {
    int b, e;
    if (basicprogram_get_line_offsets (self->bp, l, &b, &e))
      {
      tokenizer_set_pos (t, basicprogram_c_str(self->bp) + b); 
      }
    else
      {
      strings_output_string (BASIC_ERR_UNKNOWN_LINE);
      interface_output_string (": ");
      interface_output_number (l);
      interface_output_endl ();
      *error = BASIC_ERR_UNKNOWN_LINE; 
      }
    }
  }

/*===========================================================================
  parser_branch_gosub_statement
===========================================================================*/
static void parser_branch_gosub_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Slurp GOSUB

  VARTYPE l = parser_branch_expr (self, t, error);
  if (*error) return;


  // Check the stack
  if (self->gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH - 1)
    {
    int b, e;
    if (basicprogram_get_line_offsets (self->bp, l, &b, &e))
      {
      self->gosub_stack [self->gosub_stack_ptr] = tokenizer_get_pos (t);
      tokenizer_set_pos (t, basicprogram_c_str (self->bp) + b); 
      self->gosub_stack_ptr++;
      }
    else
      {
      strings_output_string (BASIC_ERR_UNKNOWN_LINE);
      interface_output_string (": ");
      interface_output_number (l);
      interface_output_endl ();
      *error = BASIC_ERR_UNKNOWN_LINE; 
      }
    }
  else
    {
    *error = BASIC_ERR_GOSUB_DEPTH;
    }
  }

/*===========================================================================
  parser_branch_return_statement
===========================================================================*/
static void parser_branch_return_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Slurp RETURN
  if (self->gosub_stack_ptr > 0)
    {
    self->gosub_stack_ptr--;
    const char *pos = self->gosub_stack [self->gosub_stack_ptr];
    tokenizer_set_pos (t, pos);
    }
  else
    {
    *error = BASIC_ERR_RETURN_WITHOUT_GOSUB;
    }
  }

/*===========================================================================
  parser_branch_if_statement
===========================================================================*/
static void parser_branch_if_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Slurp IF

  VARTYPE condition = parser_branch_expr (self, t, error);

  tokenizer_next (t, error); // Skip THEN 

  if (condition)
    {
    parser_branch_statement (self, t, error);
    }
  else
    {
    const char *word; 
    do
      {
      tokenizer_next (t, error);
      word = tokenizer_get_word (t);
      if (*error) return;
      } while (!tokenizer_is_eol (t) 
          && !strings_compare_index (word, STRING_INDEX_ELSE) && !*error);
    if (strings_compare_index (word, STRING_INDEX_ELSE))
      {
      tokenizer_next (t, error);
      parser_branch_statement (self, t, error);
      }
    else if (tokenizer_is_eol (t))
      {
      tokenizer_next (t, error);
      }
    }
  }

/*===========================================================================
  parser_branch_rem_statement
===========================================================================*/
static void parser_branch_rem_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  parser_skip_to_next_line (self, t, error);
  }

/*===========================================================================
  parser_branch_for_statement
===========================================================================*/
static void parser_branch_for_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  if (self->for_stack_ptr >= MAX_FOR_STACK_DEPTH)
    {
    *error = BASIC_ERR_FOR_DEPTH;
    return;
    }

  tokenizer_next (t, error); // Skip FOR

  char *var_name = NULL;
  if (tokenizer_is_word (t))
    {
    var_name = strdup (tokenizer_get_word(t));
    tokenizer_next (t, error);
    }
  else
    {
    *error = BASIC_ERR_NO_FOR_VAR;
    free (var_name);
    return;
    }

  if (tokenizer_is_symbol (t, '='))
    {
    tokenizer_next (t, error);
    }
  else
    {
    *error = BASIC_ERR_NO_FOR_EQ;
    free (var_name);
    return;
    }

  VARTYPE start = parser_branch_expr (self, t, error);
  if (*error) 
    {
    free (var_name);
    return;
    }

  variabletable_set_number (self->vt, var_name, start, error);
  if (*error) 
    {
    free (var_name);
    return;
    }

  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word(t);
    if (strings_compare_index (word, STRING_INDEX_TO))
      {
      tokenizer_next (t, error);
      }
    else
      {
      *error = BASIC_ERR_NO_FOR_TO;
      free (var_name);
      return;
      }
    }
  else
    {
    *error = BASIC_ERR_NO_FOR_TO;
    free (var_name);
    return;
    }

  VARTYPE end = parser_branch_expr (self, t, error);
  if (*error) 
    {
    free (var_name);
    return;
    }

  uint8_t p = self->for_stack_ptr;
  self->for_stack[p].var_name = var_name;
  self->for_stack[p].back_pos = tokenizer_get_pos (t); 
  self->for_stack[p].to = end;
  self->for_stack_ptr++;
  }

/*===========================================================================
  parser_branch_next_statement
===========================================================================*/
static void parser_branch_next_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  int p = self->for_stack_ptr;

  if (p == 0)
    {
    *error = BASIC_ERR_NEXT_WITHOUT_FOR;
    return;
    }

  tokenizer_next (t, error); // Skip NEXT

  /*
  // We aren't accepting a loop variable after NEXT at present.
  char *var_name = NULL;
  if (tokenizer_is_word (t))
    {
    var_name = strdup (tokenizer_get_word(t));
    tokenizer_next (t, error);
    }
  else
    {
    strings_output_string (STRING_INDEX_NO_NEXT_VAR);
    interface_output_endl(); 
    *error = BASIC_ERR_SYNTAX;
    free (var_name);
    return;
    }
  */

  // Find the variable in the for stack, if it's there
  // TODO TODO TODO we're only looking at the top of the stack

  char *var_name = self->for_stack[p - 1].var_name; 
  VARTYPE to = self->for_stack[p - 1].to;
  const char *pos = self->for_stack[p - 1].back_pos;
  VARTYPE count;
  if (variabletable_get_number (self->vt, var_name, &count))
    {
    // We shouldn't need to check the variable exists, since it's come
    //  off the stack.
    }

  if (count == to)
    {
    // We're done -- unwind the stack, and don't jump back
    free (self->for_stack[p - 1].var_name);
    self->for_stack_ptr--;
    }
  else
    {
    // Not done -- increment the count and jump back
    variabletable_set_number (self->vt, var_name, count + 1, error);
    if (!*error)
      {
      tokenizer_set_pos (t, pos);
      }
    }
  }

/*===========================================================================
  parser_branch_assignment
===========================================================================*/
static void parser_branch_assignment (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  // On entry, tokenizer will be over the name, if there is one
  const char *var_name = tokenizer_get_word (t);
  char *var_name_2 = strdup (var_name);
  tokenizer_next (t, error);
  if (*error) return;

  if (tokenizer_is_symbol (t, '='))
    {
    tokenizer_next (t, error);
    if (*error) 
      {
      free (var_name_2);
      return;
      }
    VARTYPE v = parser_branch_expr (self, t, error);
    if (*error) 
      {
      free (var_name_2);
      return;
      }
    variabletable_set_number (self->vt, var_name_2, v, error);
    free (var_name_2);
    }
  else
    {
    *error = BASIC_ERR_VAR_NO_EQ;
    }
  }

/*============================================================================
 * parser_parse_number
 * =========================================================================*/
VARTYPE parser_parse_number (const char *s, uint8_t *conv)
  {
  // TODO -- I really need to unify all the number conversion functions
  //  that are scattered throughout this code.
  VARTYPE r;
  *conv = basicprogram_get_line_number (s, &r);
  return r;
  }

/*===========================================================================
  parser_branch_input_statement
===========================================================================*/
static void parser_branch_input_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip INPUT
  if (*error) return;

  // INPUT should be followed by a variable

  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word (t);
    char s_num [MAX_NUMBER + 1];

    uint8_t e = 0;
    // readstring can report various errors, which we need to handle.
    // As well as reporting that the input is too long for the buffer,
    //   we also have to handle a situation where the user hits ctrl+c
    //   in the middle of input
    interface_readstring (s_num, MAX_NUMBER, &e);
    // Since we're entering a number, turn the "input too long"
    //   message into a more meaningful "number too long"
    if (e == BASIC_ERR_INPUT_TOO_LONG)
      {
      *error = BASIC_ERR_NUMBER_TOO_LONG;
      }
    if (e == 0)
      {
      uint8_t converted = 0;
      VARTYPE val = parser_parse_number (s_num, &converted);
      if (converted > 0)
        {
        variabletable_set_number (self->vt, word, val, error);
        tokenizer_next (t, error);
        }
      else
        {
        *error = BASIC_ERR_MALFORMED_NUMBER;
        }
      }
    }
  else
    {
    *error = BASIC_ERR_KW_NO_VAR;
    }
  }

/*===========================================================================
  parser_branch_millis_statement
===========================================================================*/
static void parser_branch_millis_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip MILLIS 
  if (*error) return;

  // MILLIS should be followed by a variable

  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word (t);
    VARTYPE val = interface_millis();
    variabletable_set_number (self->vt, word, val, error);
    tokenizer_next (t, error);
    }
  else
    {
    strings_output_string (BASIC_ERR_KW_NO_VAR);
    interface_output_endl(); 
    *error = BASIC_ERR_KW_NO_VAR;
    }
  }

/*===========================================================================
  parser_branch_peek_statement
===========================================================================*/
static void parser_branch_peek_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip PEEK 
  if (*error) return;

  VARTYPE address = parser_branch_expr (self, t, error);
  if (*error) return;

  if (!tokenizer_is_symbol (t, ','))
    {
    *error = BASIC_ERR_EXPECTED_COMMA;
    return;
    }

  tokenizer_next (t, error); // Skip comma 
  if (*error) return;

  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word (t);
    VARTYPE val = interface_peek (address);
    variabletable_set_number (self->vt, word, val, error);
    tokenizer_next (t, error);
    }
  else
    {
    *error = BASIC_ERR_KW_NO_VAR;
    }
  }

/*===========================================================================
  parser_branch_digitalwrite_statement
===========================================================================*/
static void parser_branch_digitalwrite_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip SETPIN
  if (*error) return;

  VARTYPE pin = parser_branch_expr (self, t, error);
  if (*error) return;

  if (!tokenizer_is_symbol (t, ','))
    {
    *error = BASIC_ERR_EXPECTED_COMMA;
    return;
    }

  tokenizer_next (t, error);
  if (*error) return;

  VARTYPE value = parser_branch_expr (self, t, error);
  if (*error) return;

  interface_digitalwrite (pin, value);
  }

/*===========================================================================
  parser_branch_analogwrite_statement
===========================================================================*/
static void parser_branch_analogwrite_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip SETPIN
  if (*error) return;

  VARTYPE pin = parser_branch_expr (self, t, error);
  if (*error) return;

  if (!tokenizer_is_symbol (t, ','))
    {
    *error = BASIC_ERR_EXPECTED_COMMA;
    return;
    }

  tokenizer_next (t, error);
  if (*error) return;

  VARTYPE value = parser_branch_expr (self, t, error);
  if (*error) return;

  interface_analogwrite (pin, value);
  }

/*===========================================================================
  parser_branch_poke_statement
===========================================================================*/
static void parser_branch_poke_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip POKE
  if (*error) return;

  VARTYPE addr = parser_branch_expr (self, t, error);
  if (*error) return;

  if (!tokenizer_is_symbol (t, ','))
    {
    *error = BASIC_ERR_EXPECTED_COMMA;
    return;
    }

  tokenizer_next (t, error);
  if (*error) return;

  VARTYPE value = parser_branch_expr (self, t, error);
  if (*error) return;

  interface_poke (addr, value);
  }

/*===========================================================================
  parser_branch_pinmode_statement
===========================================================================*/
static void parser_branch_pinmode_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip SETPIN
  if (*error) return;

  VARTYPE pin = parser_branch_expr (self, t, error);
  if (*error) return;

  if (!tokenizer_is_symbol (t, ','))
    {
    *error = BASIC_ERR_EXPECTED_COMMA;
    return;
    }

  tokenizer_next (t, error);
  if (*error) return;

  VARTYPE value = parser_branch_expr (self, t, error);
  if (*error) return;

  interface_pinmode (pin, value);
  }

/*===========================================================================
  parser_branch_analogread_statement
===========================================================================*/
static void parser_branch_analogread_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip GETPIN
  if (*error) return;

  VARTYPE address = parser_branch_expr (self, t, error);
  if (*error) return;

  if (!tokenizer_is_symbol (t, ','))
    {
    *error = BASIC_ERR_EXPECTED_COMMA;
    return;
    }

  tokenizer_next (t, error); // Skip comma 
  if (*error) return;

  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word (t);
    VARTYPE val = interface_analogread (address);
    variabletable_set_number (self->vt, word, val, error);
    tokenizer_next (t, error);
    }
  else
    {
    *error = BASIC_ERR_KW_NO_VAR;
    }
  }

/*===========================================================================
  parser_branch_digitalread_statement
===========================================================================*/
static void parser_branch_digitalread_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip GETPIN
  if (*error) return;

  VARTYPE address = parser_branch_expr (self, t, error);
  if (*error) return;

  if (!tokenizer_is_symbol (t, ','))
    {
    *error = BASIC_ERR_EXPECTED_COMMA;
    return;
    }

  tokenizer_next (t, error); // Skip comma 
  if (*error) return;

  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word (t);
    VARTYPE val = interface_digitalread (address);
    variabletable_set_number (self->vt, word, val, error);
    tokenizer_next (t, error);
    }
  else
    {
    *error = BASIC_ERR_KW_NO_VAR;
    }
  }

/*===========================================================================
  parser_branch_delay_statement
===========================================================================*/
static void parser_branch_delay_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  tokenizer_next (t, error); // Skip DELAY 
  if (*error) return;

  VARTYPE d = parser_branch_expr (self, t, error);
  interface_delay (d);
  }

/*===========================================================================
  parser_branch_statement
===========================================================================*/
static void parser_branch_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  if (interface_check_stop ())
    {
    *error = BASIC_ERR_INTERRUPTED;
    return;
    }

  if (tokenizer_is_word (t))
    {
    const char *word = tokenizer_get_word (t);
    if (strings_compare_index (word, STRING_INDEX_PRINT))
      {
      parser_branch_print_statement (self, t, error); 
      }
    else if (strcmp (word, "?") == 0)
      {
      parser_branch_print_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_IF))
      {
      parser_branch_if_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_GOTO))
      {
      parser_branch_goto_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_GOSUB))
      {
      parser_branch_gosub_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_END))
      {
      tokenizer_next (t, error);
      self->ended = TRUE;
      }
    else if (strings_compare_index (word, STRING_INDEX_RETURN))
      {
      parser_branch_return_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_REM))
      {
      parser_branch_rem_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_FOR))
      {
      parser_branch_for_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_NEXT))
      {
      parser_branch_next_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_INPUT))
      {
      parser_branch_input_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_LET))
      {
      tokenizer_next (t, error);
      if (*error) return;
      parser_branch_assignment (self, t, error);      
      }
    else if (strings_compare_index (word, STRING_INDEX_MILLIS))
      {
      parser_branch_millis_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_PEEK))
      {
      parser_branch_peek_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_DIGITALREAD))
      {
      parser_branch_digitalread_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_ANALOGREAD))
      {
      parser_branch_analogread_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_ANALOGWRITE))
      {
      parser_branch_analogwrite_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_DIGITALWRITE))
      {
      parser_branch_digitalwrite_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_PINMODE))
      {
      parser_branch_pinmode_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_POKE))
      {
      parser_branch_poke_statement (self, t, error); 
      }
    else if (strings_compare_index (word, STRING_INDEX_DELAY))
      {
      parser_branch_delay_statement (self, t, error); 
      // TODO: arduino bits 
      }
    else
      {
      // It's a word, but not a keyword. It might be "foo = 2"
      parser_branch_assignment (self, t, error);      
      }
    } 
  else
    {
    // Not a keyword or a variable
    *error = BASIC_ERR_SYNTAX;
    }
  }

/*===========================================================================
  parser_numbered_statement
===========================================================================*/
static void parser_branch_numbered_statement (Parser *self, 
         Tokenizer *t, uint8_t *error)
  {
  if (tokenizer_is_number (t))
    {
    VARTYPE r = tokenizer_get_number_value (t);
    self->current_line = r;
    tokenizer_next (t, error);
    if (*error) return;
    parser_branch_statement (self, t, error);
    }
  else
    {
    *error = BASIC_ERR_NO_LINE_NUM;
    }
  }

/*===========================================================================
  parser_run_from_pos
===========================================================================*/
static void parser_run_from_pos (Parser *self, const char *pos)
  {
  Tokenizer *t = tokenizer_new (pos);

  self->gosub_stack_ptr = 0;
  self->for_stack_ptr = 0;
  self->ended = FALSE;
  uint8_t error = 0;
  tokenizer_next (t, &error); 
  // TODO handle error 

  do
    {
    parser_branch_numbered_statement (self, t, &error);
    parser_emit_if_error (self, t, error);
    //printf ("r=%d e=%d\n", 0, error);
    if (!error)
      {
      tokenizer_next (t, &error);
      parser_emit_if_error (self, t, error);
      }
    } while (!error && !tokenizer_finished (t) && !self->ended);

  tokenizer_destroy (t);
  // Clear FOR stack in case the program did not do enough
  //  NEXTs
  for (int i = 0; i < self->for_stack_ptr; i++)
    {
    free (self->for_stack[i].var_name);
    }
  self->for_stack_ptr = 0;
  }

/*===========================================================================
  parser_run
===========================================================================*/
void parser_run (Parser *self)
  {
  self->gosub_stack_ptr = 0;
  parser_run_from_pos (self, basicprogram_c_str (self->bp));
  }

/*===========================================================================
  parser_set_variable_table
===========================================================================*/
void parser_set_variable_table (Parser *self, VariableTable *vt)
  {
  self->vt = vt;
  }

/*===========================================================================
  parser_run_line
  // Line must end in \n
===========================================================================*/
void parser_run_line (Parser *self, const char *line)
  {
  self->gosub_stack_ptr = 0;
  Tokenizer *t = tokenizer_new (line);
  uint8_t error = 0;
  tokenizer_next (t, &error);
  if (error == 0)
    parser_branch_statement (self, t, &error);
  parser_emit_if_error (self, t, error); 
  tokenizer_destroy (t);
  }

/*===========================================================================
  parser_clear_variables
===========================================================================*/
void parser_clear_variables (Parser *self)
  {
  variabletable_clear (self->vt); 
  }



