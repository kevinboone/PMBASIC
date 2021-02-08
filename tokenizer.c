/*============================================================================
  
  pmbasic

  tokenizer.c

  PMBasic tokenizer implementation. It's pretty crude, frankly.

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "defs.h"
#include "tokenizer.h"

/*===========================================================================
  Tokenizer 
===========================================================================*/

typedef uint8_t TokenType;
extern TokenType   tokenizer_get_type (const Tokenizer *self);


#define TOKEN_ERROR_NONE                0
#define TOKEN_ERROR_TOO_LONG            1
#define TOKEN_ERROR_INTERNAL            2 

#define TOKEN_TYPE_UNKNOWN        0
#define TOKEN_TYPE_NUMBER         1
#define TOKEN_TYPE_WORD           2
#define TOKEN_TYPE_STRING         3
#define TOKEN_TYPE_EOL            4
#define TOKEN_TYPE_SYM            5


struct _Tokenizer
  {
  const char *pos;
  BOOL finished;
  char current_token [TOKEN_MAX_LENGTH + 1];
  uint8_t current_token_index;
  TokenType current_token_type;
  VARTYPE number_value;
  uint16_t line;
  };

typedef uint8_t TokenClass;

/*===========================================================================
  tokenizer_new
===========================================================================*/
Tokenizer *tokenizer_new (const char *p)
  {
  Tokenizer *self = malloc (sizeof (Tokenizer));
  self->pos = p;
  self->current_token_index = 0;
  self->current_token_type = TOKEN_TYPE_UNKNOWN;
  self->finished = FALSE;
  self->line = 0;
  return self;
  }

/*===========================================================================
  tokenizer_destroy
===========================================================================*/
void tokenizer_destroy (Tokenizer *self)
  {
  free (self);
  }

/*===========================================================================
  tokenizer_add_to_token
===========================================================================*/
BOOL tokenizer_add_to_token (Tokenizer *self, char c)
  {
  if (self->current_token_index >= TOKEN_MAX_LENGTH - 2)
    return FALSE;

  self->current_token [self->current_token_index] = c;
  self->current_token_index++;
  self->current_token [self->current_token_index] = 0;

  return TRUE;
  }

/*===========================================================================
  tokenizer_slurp_string
===========================================================================*/
static int tokenizer_slurp_string (Tokenizer *self, TokenError *error)
  {
  const char *p = self->pos;
  int slurped = 0;
  char c;

  BOOL stop = FALSE;
  while (!stop && !(*error))
    {
    c = p[slurped];
    //printf ("slurpd = %d c = %c\n", slurped, c);
    if (c == '\n') 
      stop = TRUE;
    else if (c == 0) 
      stop = TRUE;
    else if (c == '\"')
      {
      // If the " is followed immediately by a ", that's an escaped "
      //   and renders as a single ". Otherwise, it's end of string
      if (p[slurped + 1] == '\"')
        {
        if (!tokenizer_add_to_token (self, '\"'))
          *error = TOKEN_ERROR_TOO_LONG; 
	slurped++;
	}
      else
        stop = TRUE;
      slurped++;
      }
    else
      {
      if (!tokenizer_add_to_token (self, c))
        *error = TOKEN_ERROR_TOO_LONG; 
      slurped++;
      }
    }
  //printf ("slurped string = %d\n", slurped);
  return slurped;
  }

/*===========================================================================
  tokenizer_slurp_word
===========================================================================*/
static int tokenizer_slurp_word (Tokenizer *self, TokenError *error)
  {
  const char *p = self->pos;
  int slurped = 0;
  char c;
  while (c = p[slurped], (isalnum (c) || c == '?') 
               && slurped < TOKEN_MAX_LENGTH)
    {
    if (!tokenizer_add_to_token (self, c))
      *error = TOKEN_ERROR_TOO_LONG; 
    slurped++;
    }
  //printf ("slurped word = %d\n", slurped);
  return slurped;
  }

/*===========================================================================
  tokenizer_slurp_decimal
===========================================================================*/
static int tokenizer_slurp_decimal (Tokenizer *self, TokenError *error)
  {
  const char *p = self->pos;
  int slurped = 0;
  char c = p[slurped];
  slurped++;
  VARTYPE total = c - '0'; 
  tokenizer_add_to_token (self, c);
  while (c = p[slurped], isdigit (c) && slurped < TOKEN_MAX_LENGTH)
    {
    if (!tokenizer_add_to_token (self, c))
      *error = TOKEN_ERROR_TOO_LONG; 
    slurped++; 
    total = 10 * total + (c - '0');
    }
  self->number_value = total;
  //printf ("slurped num = %d\n", slurped);
  return slurped;
  }

/*===========================================================================
  tokenizer_hex_digit_val
===========================================================================*/
static int tokenizer_hex_digit_val (char c)
  {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return c - 'a' + 10;
  }

/*===========================================================================
  tokenizer_slurp_hex
===========================================================================*/
static int tokenizer_slurp_hex (Tokenizer *self, TokenError *error)
  {
  const char *p = self->pos;
  int slurped = 0;
  char c = p[slurped];
  slurped++;
  VARTYPE total = tokenizer_hex_digit_val (c); 
  tokenizer_add_to_token (self, c);
  while (c = p[slurped], isxdigit (c) && slurped < TOKEN_MAX_LENGTH)
    {
    if (!tokenizer_add_to_token (self, c))
      *error = TOKEN_ERROR_TOO_LONG; 
    slurped++; 
    total = 16 * total + tokenizer_hex_digit_val (c); 
    }
  self->number_value = total;
  //printf ("slurped hex num = %d\n", slurped);
  return slurped;
  }

/*===========================================================================
  tokenizer_slurp_whitepsace
===========================================================================*/
static int tokenizer_slurp_whitespace (Tokenizer *self, TokenError *error)
  {
  (void)error;
  const char *p = self->pos;
  int slurped = 0;
  char c;
  while (c = p[slurped], c == ' ' || c == '\t')
    {
    slurped++; 
    }
  //printf ("slurped white = %d\n", slurped);
  return slurped;
  }

/*===========================================================================
  tokenizer_is_symbol_char
===========================================================================*/
static BOOL tokenizer_is_symbol_char (char c)
  {
  if (c >= '0' && c <= '9') return FALSE; 
  if (c >= 'a' && c <= 'z') return FALSE; 
  if (c >= 'A' && c <= 'Z') return FALSE; 
  if (c == 0) return FALSE;
  if (c == '\n') return FALSE;
  return TRUE; // TODO
  }

/*===========================================================================
  tokenizer_slurp_symbol
===========================================================================*/
static int tokenizer_slurp_symbol (Tokenizer *self, TokenError *error)
  {
  // This is easy, because we already know there's a symbol at pos
  //   on entry, and all symbols are one character long (so far)
  (void)error;
  tokenizer_add_to_token (self, *self->pos);
  //printf ("slurped symbol %c\n", *self->pos);
  return 1;
  }

/*===========================================================================
  tokenizer_next
===========================================================================*/
void tokenizer_next (Tokenizer *self, TokenError *error)
  {
  if (self->finished) return; // Don't waste time doing nothing
  self->current_token_index = 0;
  self->number_value = 0;
  self->current_token[0] = 0;
  char c = *self->pos;
  if (c == 0)
    {
    self->current_token_type = TOKEN_TYPE_EOL;
    self->finished = TRUE;
    }
  else if (isalpha (c) || c == '?')
    {
    self->current_token_type = TOKEN_TYPE_WORD;
    int slurped = tokenizer_slurp_word (self, error);
    if (!*error)
      self->pos += slurped;
    }
  else if (c == '#') // Do this before slurping symbols
    {
    self->pos++;
    if (isxdigit (*self->pos))
      {
      self->current_token_type = TOKEN_TYPE_NUMBER;
      int slurped = tokenizer_slurp_hex (self, error);
      if (!*error)
        self->pos += slurped;
      }
    else
      {
      self->current_token_type = TOKEN_TYPE_SYM;
      tokenizer_add_to_token (self, '#');
      }
    }
  else if (isdigit (c))
    {
    self->current_token_type = TOKEN_TYPE_NUMBER;
    int slurped = tokenizer_slurp_decimal (self, error);
    if (!*error)
      self->pos += slurped;
    }
  else if (c == ' ' || c == '\t')
    {
    int slurped = tokenizer_slurp_whitespace (self, error);
    if (!*error)
      {
      self->pos += slurped;
      tokenizer_next (self, error);
      }
    }
  else if (c == '\"') // Do this before symbol, because " would count
    {
    self->pos++; // Skip the leading " before slurping
    self->current_token_type = TOKEN_TYPE_STRING;
    int slurped = tokenizer_slurp_string (self, error);
    if (!*error)
      self->pos += slurped;
    }
  else if (tokenizer_is_symbol_char (c))
    {
    self->current_token_type = TOKEN_TYPE_SYM;
    int slurped = tokenizer_slurp_symbol (self, error);
    if (!*error)
      self->pos += slurped;
    }
  else if (c == '\n')
    {
    self->current_token_type = TOKEN_TYPE_EOL;
    self->pos += 1;
    }
  else 
    {
    *error = TOKEN_ERROR_INTERNAL;
    }
  //printf ("token type = %d %s\n", self->current_token_type, 
  //  self->current_token); 
  }

/*===========================================================================
  tokenizer_finished
===========================================================================*/
BOOL tokenizer_finished (const Tokenizer *self)
  {
  return self->finished;
  }

/*===========================================================================
  tokenizer_get_text
===========================================================================*/
const char *tokenizer_get_text (const Tokenizer *self)
  {
  return self->current_token;
  }

/*===========================================================================
  tokenizer_get_type
===========================================================================*/
TokenType tokenizer_get_type (const Tokenizer *self)
  {
  return self->current_token_type;
  }

/*===========================================================================
  tokenizer_get_line
===========================================================================*/
VARTYPE tokenizer_get_line (const Tokenizer *self)
  {
  return self->line;
  }

/*===========================================================================
  tokenizer_get_number_value
===========================================================================*/
VARTYPE tokenizer_get_number_value (const Tokenizer *self)
  {
  return self->number_value;
  }

/*===========================================================================
  tokenizer_is_number
===========================================================================*/
extern BOOL tokenizer_is_number (const Tokenizer *self)
  {
  return self->current_token_type == TOKEN_TYPE_NUMBER;
  }

/*===========================================================================
  tokenizer_is_symbol
===========================================================================*/
extern BOOL tokenizer_is_symbol (const Tokenizer *self, char sym)
  {
  return (self->current_token_type == TOKEN_TYPE_SYM
    && self->current_token[0] == sym);
  }

/*===========================================================================
  tokenizer_get_sym
===========================================================================*/
extern char tokenizer_get_sym (const Tokenizer *self)
  {
  return self->current_token[0];
  }

/*===========================================================================
  tokenizer_is_string
===========================================================================*/
extern BOOL tokenizer_is_string (const Tokenizer *self)
  {
  return (self->current_token_type == TOKEN_TYPE_STRING);
  }

/*===========================================================================
  tokenizer_get_string
===========================================================================*/
extern const char *tokenizer_get_string (const Tokenizer *self)
  {
  return self->current_token;
  }

/*===========================================================================
  tokenizer_is_word
===========================================================================*/
extern BOOL tokenizer_is_word (const Tokenizer *self)
  {
  return (self->current_token_type == TOKEN_TYPE_WORD);
  }

/*===========================================================================
  tokenizer_get_string
===========================================================================*/
extern const char *tokenizer_get_word (const Tokenizer *self)
  {
  return self->current_token;
  }

/*===========================================================================
  tokenizer_is_eol
===========================================================================*/
extern BOOL tokenizer_is_eol (const Tokenizer *self)
  {
  return (self->current_token_type == TOKEN_TYPE_EOL);
  }

/*===========================================================================
  tokenizer_is_eol
===========================================================================*/
void tokenizer_set_pos (Tokenizer *self, const char *pos)
  {
  self->pos = pos;
  }

/*===========================================================================
  tokenizer_get_pos
===========================================================================*/
const char *tokenizer_get_pos (const Tokenizer *self)
  {
  return self->pos;
  }

