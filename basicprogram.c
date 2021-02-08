/*============================================================================
  
  basicprogram.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "basicprogram.h"

#define KLOG_IN
#define KLOG_OUT

typedef struct
  {
  BOOL found;
  VARTYPE line;
  VARTYPE line2;
  const char *p1;
  const char *p2;
  } IterateLineData;

/*============================================================================
  
  BasicProgram

  ==========================================================================*/
struct _BasicProgram
  {
  char *str;
  };


/*============================================================================
  
  basicprogram_new_empty

  ==========================================================================*/
BasicProgram *basicprogram_new_empty (void)
  {
  KLOG_IN
  BasicProgram *self = malloc (sizeof (BasicProgram));
  if (self)
    {
    self->str = strdup (""); 
    }
  KLOG_OUT
  return self;
  }
  
/*============================================================================
  
  basicprogram_new

  ==========================================================================*/
BasicProgram *basicprogram_new (const char *prog)
  {
  KLOG_IN
  BasicProgram *self = malloc (sizeof (BasicProgram));
  if (self)
    {
    self->str = strdup (prog); 
    }
  KLOG_OUT
  return self;
  }
  
/*============================================================================
  
  basicprogram_set_program

  ==========================================================================*/
void basicprogram_set_program (BasicProgram *self, const char *prog)
  {
  KLOG_IN
  if (self->str) free (self->str);
  self->str = strdup (prog); 
  KLOG_OUT
  }
  
/*============================================================================
  
  basicprogram_destroy

  ==========================================================================*/
void basicprogram_destroy (BasicProgram *self)
  {
  KLOG_IN
  if (self)
    {
    free (self->str);
    free (self);
    }
  KLOG_OUT
  }

/*============================================================================
  
  basicprogram_c_str

  ==========================================================================*/
const char *basicprogram_c_str (const BasicProgram *self)
  {
  return self->str;
  }

/*============================================================================
  
  basicprogram_iterate_lines

  ==========================================================================*/
void basicprogram_iterate_lines (const BasicProgram *self, 
        BasicProgramIterator bpi, void *user_data)
  {
  const char *p1 = self->str;
  const char *p2 = strchr (p1, '\n');
  BOOL cont = TRUE;
  while (p2 && cont)
    {
    cont = bpi (self, p1, p2, user_data);
    p1 = p2 + 1;
    p2 = strchr (p1, '\n');
    }
  }

/*============================================================================
  
  basicprogram_get_line_number
  // TODO -- in theory, line numbers can be in hex. We really ought to
  //   allow for this, as the parser can.

  ==========================================================================*/
BOOL basicprogram_get_line_number (const char *line, VARTYPE *n)
  {
  BOOL ret = FALSE;
  VARTYPE total = 0;
  int i = 0;
  int c = line[i];
  while (isdigit (c) && i <= MAX_NUMBER)
    {
    ret = TRUE;
    int digit = c - '0';
    total *= 10;
    total += digit;
    i++;
    c = line[i];
    }
  *n = total;
  if (i > MAX_NUMBER) ret = FALSE;
  return ret;
  }

/*============================================================================
  
  basicprogram_get_line_offset_iterator

  ==========================================================================*/
static BOOL basicprogram_get_line_offset_iterator (const BasicProgram *self, 
              const char *p1, const char *p2, void *user_data)
  {
  KLOG_IN
  BOOL ret = TRUE;
  self = self;
  VARTYPE n;
  if (basicprogram_get_line_number (p1, &n))
    {
    if (n == ((IterateLineData*)user_data)->line)
      {
      ((IterateLineData*)user_data)->found = TRUE;
      ((IterateLineData*)user_data)->p1 = p1;
      ((IterateLineData*)user_data)->p2 = p2;
      ret = FALSE;
      }
    }
  else
    {
    // Skip malformed lines for now
    }
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  basicprogram_get_line_offsets

  ==========================================================================*/
BOOL basicprogram_get_line_offsets (const BasicProgram *self, 
              VARTYPE line, int *begin, int *end)
  {
  KLOG_IN
  BOOL ret =FALSE;
  IterateLineData ild;
  ild.line = line;
  ild.found = FALSE;
  basicprogram_iterate_lines (self, 
           basicprogram_get_line_offset_iterator, &ild);
  if (ild.found)
    {
    *begin = ild.p1 - self->str;
    *end= ild.p2 - self->str;
    ret = TRUE;
    }

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  basicprogram_delete_range

  Delete n characters starting at b. If n > length, delete from b to end. 

  ==========================================================================*/
static void basicprogram_delete_range (BasicProgram *self, int b, int n)
  {
  KLOG_IN
  char *str = self->str;
  int lself = strlen (str); 
  if (b + n > lself)
    basicprogram_delete_range (self, b, lself - n);
  else
    {
    memmove (str + b, str + b + n + 1, lself - (b + n));
    lself -= n + 1;
    str[lself] = 0;
    self->str = realloc (self->str, lself + 1);
    }
  KLOG_OUT
  }


/*============================================================================
  
  basicprogram_delete_line

  ==========================================================================*/
BasicProgramResult basicprogram_delete_line (BasicProgram *self, VARTYPE n)
  {
  KLOG_IN
  BasicProgramResult ret;
  int b, e;
  if (basicprogram_get_line_offsets (self, 
              n, &b, &e))
    {
    basicprogram_delete_range (self, b, e - b);
    ret = BASICPROGRAM_LINE_DELETED;
    }
  else
    ret = BASICPROGRAM_BAD_LINE_NUMBER;

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  basicprogram_get_text_offset_of_line 
 
  Gets the start of the line text, after the number. 
  If there line contains only a number, or not even a number, returns -1. 
  Note that a line can legitimately consist only of whitespace.

  ==========================================================================*/
static int basicprogram_get_text_offset_of_line (const char *line)
  {
  KLOG_IN
  int ret = -1;

  const char *sp = strchr (line, ' '); 
  if (!sp)
    sp = strchr (line, '\t'); 
  if (sp)
    {
    return sp - line;
    }

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  basicprogram_get_next_line_up_iterator

  ==========================================================================*/
static BOOL basicprogram_get_next_line_up_iterator (const BasicProgram *self, 
              const char *p1, const char *p2, void *user_data)
  {
  KLOG_IN
  BOOL ret = TRUE;
  p2 = p2;
  self = self;
  VARTYPE n;
  if (basicprogram_get_line_number (p1, &n))
    {
    if (n > ((IterateLineData *)user_data)->line)
      {
      ((IterateLineData *)user_data)->line2 = n;
      ((IterateLineData *)user_data)->found = TRUE;
      ret = FALSE;
      }
    }
  else
    {
    // Skip malformed lines for now
    }
  KLOG_OUT
  return ret;
  }


/*============================================================================
  
  basicprogram_get_next_line_up

  ==========================================================================*/
static BOOL basicprogram_get_next_line_up (const BasicProgram *self, 
         VARTYPE n, VARTYPE *next)
  {
  KLOG_IN
  BOOL ret = FALSE;
  IterateLineData ild;
  ild.found = FALSE;
  ild.line = n;
  basicprogram_iterate_lines (self, 
           basicprogram_get_next_line_up_iterator, &ild);
  if (ild.found)
    {
    *next = ild.line2;
    ret = TRUE;
    }
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  basicprogram_insert_at_pos

  ==========================================================================*/
void basicprogram_insert_at_pos (BasicProgram *self, int pos, const char *line)
  {
  KLOG_IN
  int lself = strlen (self->str); 
  int lline = strlen (line); 
  // If pos is too large, insert at end
  if (pos > lself - 1) pos = lself - 1;
  if (pos < 0) pos = 0;

  int newsize = lself + lline + 1;

  self->str = realloc (self->str, newsize + 1);

  memmove (self->str + pos + lline, self->str + pos, lself - pos + 1);
  memmove (self->str + pos, line, lline);
  KLOG_OUT
  }

/*============================================================================
  
  basicprogram_insert_line

  ==========================================================================*/
BasicProgramResult basicprogram_insert_line (BasicProgram *self, 
                    const char *line)
  {
  KLOG_IN
  BasicProgramResult ret = BASICPROGRAM_UNCHANGED;
  VARTYPE n;
  if (basicprogram_get_line_number (line, &n))
    {
    VARTYPE to = basicprogram_get_text_offset_of_line (line);
    if (to < 0) 
      {
      ret = basicprogram_delete_line (self, n);
      }
    else
      {
      // This is a line to insert, but does the line already exist?
      int dummy;
      if (basicprogram_get_line_offsets (self, 
              n, &dummy, &dummy))
        {
        // The line exists. So delete it can call this method again
        basicprogram_delete_line (self, n);
        basicprogram_insert_line (self, line);
	ret = BASICPROGRAM_LINE_REPLACED;
        }
      else
        {
        // We're adding a line that doesn't already exist
        VARTYPE n2;
        if (basicprogram_get_next_line_up (self, n, &n2))
          {
          int b, e;
          if (basicprogram_get_line_offsets (self, 
              n2, &b, &e))
            {
            basicprogram_insert_at_pos (self, b, "\n");
            basicprogram_insert_at_pos (self, b, line);
	    ret = BASICPROGRAM_LINE_INSERTED;
            }
          else
            {
            // Should never happen
            }
          }
        else
          {
	  // Not existing line than which this is a lower number --
	  //   insert at end (ie., between the last char and the zero)
	  int l = strlen (self->str);
          basicprogram_insert_at_pos (self, l, "\n");
          basicprogram_insert_at_pos (self, l, line);
	  ret = BASICPROGRAM_LINE_APPENDED;
          }
        }
      }
    }
  else
    ret = BASICPROGRAM_BAD_LINE_NUMBER;
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  basicprogram_clear

  ==========================================================================*/
void basicprogram_clear (BasicProgram *self)
  {
  free (self->str);
  self->str = strdup ("");
  }

/*============================================================================
  
  basicprogram_get_length

  ==========================================================================*/
int basicprogram_get_length (const BasicProgram *self)
  {
  return strlen (self->str);
  }

/*============================================================================
  
  basicprogram_add_char

  ==========================================================================*/
BOOL basicprogram_add_char (BasicProgram *self, char c)
  {
  BOOL ret = FALSE;

  int cur_len = strlen (self->str); // UGH! We should store the length
  self->str = realloc (self->str, cur_len + 2);
  if (self->str)
    {
    self->str [cur_len] = c;
    cur_len++;
    self->str [cur_len] = 0;
    ret = TRUE;
    }
  else
    self->str = strdup (""); // Don't leave the memory in a mess
  return ret;
  }



