/*===========================================================================

  pmbasic

  variable.c

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "defs.h"
#include "config.h"
#include "klist.h"
#include "variable.h"

/*===========================================================================
  Variable
===========================================================================*/
struct _Variable
  {
  char *name;
  VARTYPE num_value;
  //char *str_value; // Future use
  };


/*===========================================================================
  variable_new_number
===========================================================================*/
Variable *variable_new_number (const char *name, VARTYPE number)
  {
  Variable *self = malloc (sizeof (Variable));
  if (self)
    {
    self->name = strdup (name);
    self->num_value = number;
    //self->str_value = NULL; // Future use
    }
  return self;
  }

/*===========================================================================
  variable_destroy
===========================================================================*/
void variable_destroy (Variable *self)
  {
  // if (self->str_value) free (self->str_value); // Future use
  if (self->name) free (self->name);
  free (self);
  }

/*===========================================================================
  variable_get_name
===========================================================================*/
extern const char *variable_get_name (const Variable *self)
  {
  return self->name;
  }

/*===========================================================================
  variable_set_number
===========================================================================*/
extern void variable_set_number (Variable *self, VARTYPE number)
  {
  self->num_value = number;
  }

/*===========================================================================
  variable_get_number
===========================================================================*/
VARTYPE variable_get_number (const Variable *self)
  {
  return self->num_value;
  }




