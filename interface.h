/*===========================================================================
  
  pmbasic

  interface.h

  This header defines functions that an interface must expose, and which
  will be called by the BASIC interpreter. Not all functions make sense
  in all interfaces, in which case they should just output a helpful
  message, or simply ignore them.
 
===========================================================================*/

#pragma once

#include "defs.h"
#include "config.h"
#include "basicprogram.h"

BEGIN_DECLS

extern BOOL    interface_check_stop (void);
extern void    interface_output_number (VARTYPE i);
extern void    interface_output_string (const char *s);
extern void    interface_output_endl (void);
extern void    interface_readstring (char *buff, int len, uint8_t *error);
extern VARTYPE interface_millis (void);
extern void    interface_delay (VARTYPE msec);
extern void    interface_poke (int addr, uint8_t byte);
extern uint8_t interface_peek (int addr);
extern void    interface_analogwrite (uint8_t pin, VARTYPE value);
extern void    interface_digitalwrite (uint8_t pin, uint8_t value);
extern void    interface_pinmode (uint8_t pin, uint8_t mode);
extern VARTYPE interface_analogread (uint8_t pin);
extern uint8_t interface_digitalread (uint8_t pin);
extern BOOL    interface_load (BasicProgram *bp);
extern BOOL    interface_save (const BasicProgram *bp);
extern void    interface_help (void);
extern void    interface_info (void);

END_DECLS


