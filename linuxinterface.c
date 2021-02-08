/*===========================================================================

  pmbasic

  linuxinterface.c

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/time.h> 
#include "interface.h"
#include "errcodes.h"

extern void pmbasic_main_loop (void);

/*===========================================================================
  interface_output_string
===========================================================================*/
void interface_output_string (const char *s)
  {
  printf ("%s", s);
  }

/*===========================================================================
  interface_output_number
===========================================================================*/
void interface_output_number (VARTYPE i)
  {
  printf ("%d", i);
  }

/*===========================================================================
  interface_output_endl
===========================================================================*/
void interface_output_endl (void)
  {
  printf ("\n");
  }

/*===========================================================================
  interface_readstring
===========================================================================*/
void interface_readstring (char *buff, int len, uint8_t *error)
  {
  *error = 0;
  int pos = 0;
  int c = getchar() ;
  if (c < 0) exit(0); // Nasty!
  int i = 0;
  while (c > 0 && c != 10) 
    {
    if (i < len)
      buff[pos++] = c;
    c = getchar() ;
    i++;
    }
  buff[pos] = 0;
  if (i > len) *error = BASIC_ERR_INPUT_TOO_LONG;
  }

/*===========================================================================
  interface_check_stop
===========================================================================*/
BOOL interface_check_stop (void)
  {
  return FALSE;  // Not implemented
  }

/*============================================================================
 * interface_millis 
 * =========================================================================*/
VARTYPE interface_millis (void)
  {
  struct timeval tv;
  gettimeofday (&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
  }

/*============================================================================
 * interface_delay
 * =========================================================================*/
void interface_delay (VARTYPE msec)
  {
  usleep (1000 * msec);
  }

/*============================================================================
 * interface_poke
 * =========================================================================*/
void interface_poke (int address, uint8_t byte)
  {
  printf ("POKE %d, %d -- not implemented on this platform\n",
    address, (int)byte);
  }

/*============================================================================
 * interface_peek
 * =========================================================================*/
uint8_t interface_peek (int address)
  {
  printf ("PEEK %d -- not implemented on this platform\n",
    address);
  return 0;
  }

/*============================================================================
 * interface_digitalwrite
 * =========================================================================*/
void interface_digitalwrite (uint8_t pin, uint8_t value)
  {
  (void)pin;
  (void)value;
  printf ("DIGITALWRITE %d,%d -- not implemented on this platform\n", 
    pin, value);
  }

/*============================================================================
 * interface_digitalread
 * =========================================================================*/
uint8_t interface_digitalread (uint8_t pin)
  {
  (void)pin;
  printf ("DIGITALREAD %d -- not implemented on this platform\n", pin);
  return 0;
  }

/*============================================================================
 * interface_pinmode
 * =========================================================================*/
void interface_pinmode (uint8_t pin, uint8_t mode)
  {
  (void)pin;
  (void)mode;
  printf ("PINMODE %d,%d - not implemented on this platform\n", pin, mode);
  }

/*============================================================================
 * interface_info
 * =========================================================================*/
void interface_info (void)
  {
  // Not implemented
  }

/*============================================================================
 * interface_save
 * =========================================================================*/
BOOL interface_save (const BasicProgram *bp)
  {
  (void) bp;
  printf ("SAVE not implemented\n");
  return FALSE;
  }

/*============================================================================
 * interface_load
 * =========================================================================*/
BOOL interface_load (BasicProgram *bp)
  {
  (void) bp;
  printf ("LOAD not implemented\n");
  return FALSE;
  }

/*===========================================================================
  main 
===========================================================================*/
int main ()
  {
  pmbasic_main_loop ();
  }


