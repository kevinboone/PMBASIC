/*===========================================================================

  pmbasic

  parser.h

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/

#pragma once

#include "defs.h"
#include "config.h"
#include "basicprogram.h"
#include "variabletable.h"

struct _Parser;
typedef struct _Parser Parser;

BEGIN_DECLS

extern Parser     *parser_new (void);
extern void        parser_destroy (Parser *self);

extern BOOL        parser_set_program (Parser *self, const BasicProgram *bp);
extern void        parser_set_variable_table (Parser *self, VariableTable *vt);
extern void        parser_run (Parser *self);

extern void        parser_run_line (Parser *self, const char *line);
extern void        parser_clear_variables (Parser *self);

END_DECLS

