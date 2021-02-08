/*===========================================================================

  pmbasic

  arduinointerface.c

  Implementations of the platform-specific functions defined in interface.h

  (c)2021 Kevin Boone, GPLv3.0

===========================================================================*/

/*
For the record, on atmega32u4:
DDRB=0x24=36
PORTB=0x25=37
DDRD=0x2a=42
portd=0x2b=43
RXLED is bit 0 of PORTB 
TXLED is bit 5 of PORTD 

These can be used with PEEK and POKE, so we can do a blinky like this:

10 poke #24,1
20 poke #25,1
30 delay 1000
40 poke #25,0
50 delay 1000
60 goto 10

Of course, since the RXLED is pin 17, we can do this, which is more
elegant:

10 p=17
20 pinmode p,1
30 digitalwrite p,1
40 delay 1000
50 digitalwrite p,0
60 delay 1000
70 goto 10

*/

#include <Arduino.h>
#include <HardwareSerial.h>
#include <stdlib.h> 
#include <EEPROM.h>
#include "interface.h"
#include "arduinointerface.h"
#include "errcodes.h"
#include "config.h"
#include "strings.h"

extern "C" void pmbasic_main_loop (void);

char buff[MAX_LINE + 1];

/*============================================================================
 * get_free_memory
 * There's no officially-supported way to do this and, of course, it
 *   depends on the specific run-time library in use. The code below 
 *   seems to produce credible results, but there's no way to ensure
 *   it will continue to, since it uses undocumented features of the AVR
 *   library. 
 * =========================================================================*/
extern unsigned int __heap_start;
extern void *__brkval;

struct __freelist 
  {
  size_t sz;
  struct __freelist *nx;
  };

extern struct __freelist *__flp;

static int free_list_size (void) 
  {
  struct __freelist* current;
  int total = 0;
  for (current = __flp; current; current = current->nx) 
    {
    total += 2; /* block header  */
    total += (int) current->sz;
    }
  return total;
  }

static int get_free_memory () 
 {
 int free_memory;
 if ((int)__brkval == 0) 
   {
   free_memory = ((int)&free_memory) - ((int)&__heap_start);
   } 
 else 
   {
   free_memory = ((int)&free_memory) - ((int)__brkval);
   free_memory += free_list_size();
   }
 return free_memory;
 }


/*===========================================================================
  interface_output_string
===========================================================================*/
void interface_output_string (const char *msg)
  {
  Serial.print (msg);
  }

/*===========================================================================
  interface_output_number
===========================================================================*/
void interface_output_number (VARTYPE i)
  {
#if VARTYPE == long
  sprintf (buff, "%ld", i);
#else
  sprintf (buff, "%d", i);
#endif
  Serial.print (buff);
  }

/*===========================================================================
  interface_output_endl
===========================================================================*/
void interface_output_endl (void)
  {
  Serial.print (AI_ENDL);
  }

/*===========================================================================
  interface_readstring
===========================================================================*/
void interface_readstring (char *buff, int len, uint8_t *error)
  {
  *error = 0;
  int pos = 0;
  while (!Serial.available());
  int cc =  Serial.read();
  while (cc > 0 && cc != 13 && cc != AI_INTR) 
    {
    if (cc == AI_BACKSPACE)
      {
      if (pos > 0) 
        {
	pos--;
        Serial.write (AI_BACKSPACE);
#ifdef AI_EMIT_DESTRUCTIVE_BACKSPACE
        Serial.write (' ');
        Serial.write (AI_BACKSPACE);
#endif
        }
      }
    else
      {
      if (pos < len - 1)
        {
        buff[pos++] = cc;
        Serial.write (cc);
	}
      }
    while (!Serial.available());
    cc = Serial.read() ;
    }
  buff[pos] = 0;
  interface_output_endl ();
  if (cc == AI_INTR) 
    *error = BASIC_ERR_INTERRUPTED;
  }

/*============================================================================
 * interface_check_stop
 * =========================================================================*/
BOOL interface_check_stop (void)
  {
  if (Serial.available())
    {
    if (Serial.read() == AI_INTR)
      return TRUE;
    }
  return FALSE;
  }

/*============================================================================
 * interface_millis 
 * =========================================================================*/
VARTYPE interface_millis (void)
  {
  return (VARTYPE) millis();
  }

/*============================================================================
 * interface_delay
 * =========================================================================*/
void interface_delay (VARTYPE msec)
  {
  delay (msec); 
  }

/*============================================================================
 * interface_poke
 * =========================================================================*/
void interface_poke (int address, uint8_t byte)
  {
  *((volatile uint8_t *)address) = byte;
  }

/*============================================================================
 * interface_peek
 * =========================================================================*/
uint8_t interface_peek (int address)
  {
  return *((volatile uint8_t *)address);
  }

/*============================================================================
 * interface_digitalwrite
 * =========================================================================*/
void interface_digitalwrite (uint8_t pin, uint8_t value)
  {
  digitalWrite (pin, value);
  }

/*============================================================================
 * interface_digitalread
 * =========================================================================*/
uint8_t interface_digitalread (uint8_t pin)
  {
  return digitalRead (pin);
  }

/*============================================================================
 * interface_analogwrite
 * =========================================================================*/
void interface_analogwrite (uint8_t pin, VARTYPE value)
  {
  analogWrite (pin, value);
  }


/*============================================================================
 * interface_analogread
 * =========================================================================*/
VARTYPE interface_analogread (uint8_t pin)
  {
  return analogRead (pin);
  }


/*============================================================================
 * interface_pinmode
 * =========================================================================*/
void interface_pinmode (uint8_t pin, uint8_t mode)
  {
  pinMode (pin, mode);
  }

/*============================================================================
 * interface_info
 * =========================================================================*/
void interface_info (void)
  {
  strings_output_string (STRING_INDEX_TOT_EEPROM);
  interface_output_number (EEPROM.length());
  interface_output_string (" ");
  strings_output_string (STRING_INDEX_BYTES);
  interface_output_endl ();
  strings_output_string (STRING_INDEX_FREE_RAM);
  interface_output_number (get_free_memory());
  interface_output_string (" ");
  strings_output_string (STRING_INDEX_BYTES);
  interface_output_endl ();
  // TODO
  }

/*============================================================================
 * interface_save
 * =========================================================================*/
BOOL interface_save (const BasicProgram *bp)
  {
  BOOL ret = FALSE;

  int eeprom_len = EEPROM.length ();
  int prog_len = basicprogram_get_length (bp); 
  if (prog_len < eeprom_len - 3)
    {
    int i = 0;
    EEPROM.write (i++, 'P');
    EEPROM.write (i++, 'M');
    EEPROM.write (i++, 'B');
    const char *ptr = basicprogram_c_str (bp);
    for (i = 0; i <= prog_len; i++) // Include the final zero
      EEPROM.write (i + 3, ptr[i]);
    ret = TRUE;
    }
  else
    {
    strings_output_string (BASIC_ERR_PROGRAM_TOO_LARGE);
    interface_output_endl();
    }

  return ret;
  }

/*============================================================================
 * interface_load
 * =========================================================================*/
BOOL interface_load (BasicProgram *bp)
  {
  BOOL ret = FALSE;

  char c1 = EEPROM.read (0);
  char c2 = EEPROM.read (1);
  char c3 = EEPROM.read (2);

  if (c1 == 'P' && c2 == 'M' && c3 == 'B')
    {
    basicprogram_clear (bp);
    int i = 3;
    char c = EEPROM.read (i);
    BOOL ok = TRUE;
    while (c && ok)
      {
      i++;
      ok = basicprogram_add_char (bp, c);
      c = EEPROM.read (i);
      }
    if (ok)
      ret = TRUE;
    else
      {
      strings_output_string (BASIC_ERR_NOMEM);
      interface_output_endl();
      }
    }
  else
    {
    strings_output_string (BASIC_ERR_NO_STORED_PROGRAM);
    interface_output_endl();
    }

  return ret;
  }

/*============================================================================
 * setup 
 * =========================================================================*/
void setup()
  {
  Serial.begin (57600);
  }

/*============================================================================
 * loop 
 * =========================================================================*/
void loop()
  {
  pmbasic_main_loop();
  }


