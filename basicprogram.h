/*============================================================================
  
  basicprogram.h

  Definition of the BasicProgram class

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#pragma once

#include "defs.h"
#include "config.h"

struct BasicProgram;
#ifndef __cplusplus
typedef struct _BasicProgram BasicProgram;
#endif

typedef BOOL (*BasicProgramIterator)(const BasicProgram *self, 
                 const char *b, const char *e, void *user_data);

typedef enum 
  {
  // The operation did not change the program
  BASICPROGRAM_UNCHANGED = 0, 
  // The operation failed because the caller supplied an invalid line no.
  BASICPROGRAM_BAD_LINE_NUMBER,
  // The operation resulted in a line being deleted (e.g., a line
  //   was specified that contained only a line numer)
  BASICPROGRAM_LINE_DELETED,
  // A line was replaced with a new one
  BASICPROGRAM_LINE_REPLACED,
  BASICPROGRAM_LINE_APPENDED,
  BASICPROGRAM_LINE_INSERTED
  } BasicProgramResult;

BEGIN_DECLS

/** Create an empty program (that is, a zero-length string). */
extern BasicProgram *basicprogram_new_empty (void);

/** Create the object from an existing basic program text, which is
 *   assumed to be well-formed -- that is, lines starting with numbers, 
 *   separated by \n characters. DO NOT USE EXCEPT FOR TESTING!*/
extern void          basicprogram_set_program (BasicProgram *self, 
                        const char *prog);

extern void          basicprogram_destroy (BasicProgram *self);

extern const char   *basicprogram_c_str (const BasicProgram *self);

/** Find the offsets of the line in the program. The offsets are of the 
 *   first character of the line, and the terminating \n character. The
 *   line number is not the simple count, but the number stored in the
 *   line itself, which can be completely different. */
extern BOOL          basicprogram_get_line_offsets (const BasicProgram *self, 
                       VARTYPE line, int *begin, int *end);

/** Parse the initial line number from the line. If the line doesn't start
 *   with a number, return FALSE. The line is expected to be 
 *   separated from the rest of the line by some whitespace. */
extern BOOL          basicprogram_get_line_number (const char *line, 
                        VARTYPE *n);

/** Delete the line numbered n. If there is no such line, do nothing. */
extern BasicProgramResult basicprogram_delete_line (BasicProgram *self, 
                        VARTYPE n);


/** Insert a line at the appropriate point in the program. The line is
 *    assumed to start with a valid line number. If this number matches
 *    an existing line, the old line is replaced. If the line consists
 *    only of a line number and not text, the line is deleted completely.
 *    The line should _not_ end with a CR -- one will be inserted.
 *    "insert_line" is an unhelpful, name, but 
 *    "insert_delete_replace_append_or_insert_line" would be unweildy.*/
extern BasicProgramResult basicprogram_insert_line (BasicProgram *self, 
                       const char *line);

/** Iterate lines in the program, calling the specified iterator for each
 *    line. The iterator receives the offsets of each line. */
extern void          basicprogram_iterate_lines (const BasicProgram *self, 
                         BasicProgramIterator bpi, void *user_data);

/** Clear the whole program without warning. */
extern void          basicprogram_clear (BasicProgram *self);

/** Get the length of the program, not including any final zeros. */
extern int           basicprogram_get_length (const BasicProgram *self);

/** Add a single character to the end of the program. We need to be able
 *   to do this so we can stream data out of the EEPROM. EEPROM is not
 *   memory-mapped, and we don't have enough RAM to buffer it anywhere.
 *   So we have to be able to add it character-by-character. This is slow,
 *   if course, but it won't happen very often. Returns FALSE on error,
 *   which can only mean out-of-memory. */
extern BOOL          basicprogram_add_char (BasicProgram *self, char c);

END_DECLS

