/*============================================================================
 * arduino_interface.h
 *
 * This file contains definitions that control how the Arduino interacts
 * with a terminal.
 *
 * Copyright (c)2021 Kevin Boone.
 * =========================================================================*/

#pragma once

// Define the ASCII code that the terminal will transmit to erase
//  the last character.
#define AI_BACKSPACE 8

// Serial code that will interrupt a running program. 3 = ctrl+c
#define AI_INTR 3

// Define this if your terminal does not rub out the previous character
//   when it receives a backspace from the Arduino. This setting has the
//   effect that PMBASIC will send backspace-space-backspace.
#define AI_EMIT_DESTRUCTIVE_BACKSPACE

// Define how PMBASIC will send a end-of-line to the terminal. \n=10,
//  \r=13. Some terminals automatically expand a \n into \r\n, but most
//  do not by default.
#define AI_ENDL "\r\n"
