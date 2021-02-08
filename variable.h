/*===========================================================================

  pmbasic

  variable.h

  This class represents a single variable which, for now, has only
  numeric type

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/

#pragma once

#include "defs.h"
#include "config.h"

struct _Variable;
typedef struct _Variable Variable;

BEGIN_DECLS

extern Variable   *variable_new_number (const char *name, VARTYPE number);
extern void        variable_destroy (Variable *self);

extern const char *variable_get_name (const Variable *self);
extern VARTYPE     variable_get_number (const Variable *self);
extern void        variable_set_number (Variable *self, VARTYPE number);

END_DECLS

