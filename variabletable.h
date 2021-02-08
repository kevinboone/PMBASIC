/*===========================================================================

  pmbasic

  variabletable.h

  This class represents the variable table. It is implemented as a KList
  of pointers to Variable instances.

  Some of these functions return error codes -- these valuee must be
  one of the constants defined in errcodes.h.

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/

#pragma once

#include "defs.h"
#include "config.h"
#include "variable.h"

struct _VariableTable;
typedef struct _VariableTable VariableTable;

BEGIN_DECLS

extern VariableTable *variabletable_new_empty (void);
extern void     variabletable_destroy (VariableTable *self);
extern void     variabletable_set_number (VariableTable *self, 
                          const char *name, VARTYPE number, uint8_t *error);
extern Variable *variabletable_get_variable 
                         (const VariableTable *self, const char *name);
extern BOOL     variabletable_get_number (const VariableTable *self,
                          const char *name, VARTYPE *value);
extern void     variabletable_clear (VariableTable *self);
END_DECLS
