/*===========================================================================

  pmbasic

  tokenizer.h

===========================================================================*/

#pragma once

#include "defs.h"
#include "config.h"

struct _Tokenizer;
typedef struct _Tokenizer Tokenizer;

typedef uint8_t TokenError;

BEGIN_DECLS

extern Tokenizer  *tokenizer_new (const char *p);
extern void        tokenizer_destroy (Tokenizer *self);

extern void        tokenizer_next (Tokenizer *self, TokenError *error);
extern BOOL        tokenizer_finished (const Tokenizer *self);

extern const char *tokenizer_get_text (const Tokenizer *self);

// Note that get_line here refers to a line in the program code, not
//  the number _of_ a line. We're only tokenizing here, not parsing
extern VARTYPE     tokenizer_get_line (const Tokenizer *self);

extern VARTYPE     tokenizer_get_number_value (const Tokenizer *self);

extern BOOL        tokenizer_is_number (const Tokenizer *self);

extern BOOL        tokenizer_is_symbol (const Tokenizer *self, char sym);
extern char        tokenizer_get_sym (const Tokenizer *self);

extern BOOL        tokenizer_is_word (const Tokenizer *self);
extern const char *tokenizer_get_word (const Tokenizer *self);

extern BOOL        tokenizer_is_string (const Tokenizer *self);
extern const char *tokenizer_get_string (const Tokenizer *self);

extern BOOL        tokenizer_is_eol (const Tokenizer *self);

extern void        tokenizer_set_pos (Tokenizer *self, const char *pos);
extern const char *tokenizer_get_pos (const Tokenizer *self);

END_DECLS
