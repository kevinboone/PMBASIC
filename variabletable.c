/*===========================================================================

  pmbasic

  variabletable.c

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "config.h"
#include "defs.h"
#include "config.h"
#include "klist.h"
#include "variabletable.h"
#include "variable.h"
#include "errcodes.h"

/*===========================================================================
  VariableTable 
===========================================================================*/
struct _VariableTable
  {
  KList *list; // Of Variable
  };


/*===========================================================================
  variabletable_new_empty
===========================================================================*/
VariableTable *variabletable_new_empty (void)
  {
  VariableTable *self = malloc (sizeof (VariableTable));
  if (self)
    {
    self->list = klist_new_empty ((KListFreeFn)variable_destroy);
    }
  return self;
  }

/*===========================================================================
  variabletable_destroy
===========================================================================*/
void variabletable_destroy (VariableTable *self)
  {
  if (self->list) klist_destroy (self->list);
  free (self);
  }

/*===========================================================================
  variabletable_set_number
===========================================================================*/
void variabletable_set_number (VariableTable *self, const char *name, 
        VARTYPE number, uint8_t *error)
  {
  // TODO check if name exists
  Variable *v = variabletable_get_variable (self, name);
  if (v)
    {
    variable_set_number (v, number);
    }
  else
    {
    Variable *v = variable_new_number (name, number);
    if (v)
      klist_append (self->list, v);
    else
      *error = BASIC_ERR_NOMEM;
    }
  }

/*===========================================================================
  variabletable_get_variable
===========================================================================*/
Variable *variabletable_get_variable (const VariableTable *self, 
         const char *name)
  {
  int l = klist_length (self->list);
  for (int i = 0; i < l; i++)
    {
    Variable *v = klist_get (self->list, i);
    if (strcmp (name, variable_get_name (v)) == 0) return v;
    }
  return NULL;
  }

/*===========================================================================
  variabletable_get_variable
===========================================================================*/
BOOL variabletable_get_number (const VariableTable *self,
                          const char *name, VARTYPE *value)
  {
  Variable *v = variabletable_get_variable (self, name);
  if (v)
    {
    *value = variable_get_number (v);
    return TRUE;
    }
  else
    return FALSE;
  }

/*===========================================================================
  variabletable_clear
===========================================================================*/
void variabletable_clear (VariableTable *self)
  {
  klist_clear (self->list);
  }


