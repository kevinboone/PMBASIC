# PMBASIC

A basic interpreter and editor for the SparkFun Pro Micro microcontroller,
and other Arduino-type devices.

## What is this?

PMBASIC is an implementation of a simple BASIC-like language and
minimally-interactive editor, designed to be compact enough to fit (easily)
into the 32k flash memory of a Pro Micro or similar Arduino-like
microcontroller.  It's not the flash size that's the problem in this
implementation, but the RAM -- see the Technical Details section for an
explanation why RAM is even more of a constraint than you might first think.

PMBASIC uses the Arduino USB port for communication with a serial terminal, or
terminal emulator. Since it has only a line-by-line text editing mode, the
terminal can be very unsophisticated.  Simplicity and memory-efficiency are key
goals in the design, as these microcontroller devices usually only have 2kB of
RAM or thereabouts. Naturally, it won't be possible to implement a substantial
program, given the constraints.

Connected to a terminal, the Pro Micro can offer the same kind of
BASIC-language capabilities that were found in desktop computers from the
mid-1970s. ;)

PMBASIC has a number of Arduino-specific features -- for example, 
it can read and set I/O pins, and create millisecond delays, among
other things. Programs can be saved to EEPROM, in the absence of
any other kind of storage.

## 16-bit and 32-bit builds

Change the value of `VARTYPE` in `config.h` to set the overall
mode of operation of PMBASIC. This change affects all data manipulated
by PMBASIC -- arithmetic, variables, line numbers, etc. Of course, 
32-bits is way too large for many simple applications -- the AVRs
only have a 16-bit address space, for example. However, you can't do
much useful arithmetic in 16 bits. Although PMBASIC has no floating-point
support, 32-bit arithmetic gives enough range to simulate it by,
for example, doing calculations in units of thousandths.  

In addition, using 16-bit mode will affect things like millisecond
timers, which have to count large numbers. Perhaps surprisingly, given
the extra arithmetic complexity, using 32-bit mode does not make PMBASIC
much larger -- not in terms of flash storage, anyway. However, it makes
significantly more demands on RAM. Even though PMBASIC can only store
26 variables, in 32-bit mode that could still amount to 10% of the
total available RAM. Since GOTO and GOSUB statements take line numbers,
using 32-bit mode means allowing for very large line numbers -- even
though in practice it's impossible to store more than about 50 lines
of program. 

There's really no easy way to choose between 16-bit and 32-bit mode,
nor any way to change the setting at runtime. It's a once-for-all decision
at build time.

## The language 

BASIC is a well-documented language. This implementation supports
the following features.

- The usual PRINT statement
- FOR ... NEXT loops, which can be nested
- GOTO and (nested) GOSUB constructs
- IF ... THEN ... ELSE
- Arithmetic expressions of complexity limited only by RM
- Decimal and hexadecimal numbers
- PEEK and POKE, for setting memory directly
- Arduino-specific statements PINMODE, MILLIS, DELAY... 
- Variables names of arbitrary length (subject to memory) 

### Lines

The longest line that PMBASIC can handle is defined in `config.h`. 
The default is 80 characters plus the terminating carriage return. This is not
very long, but it's long enough to edit on a dumb terminal. The longest string
that can be printed is 40 characters. Again, this can be changed in `limits.h`.

### Whitespace

Whitespace within a line is mostly ignored. You can enter whitespace
between the line number and the statement text to improve readability.
A line can legitimately consist only of whitespace (but it must
still be numbered). Given the tiny memory available, care must
be taken when using whitespace and comments. It's legitimate to
have no whitespace at all between the line number and the statement
text, but this is highly unreadable.

### Comments

Anything after `REM`, to the end of the line, is ignored. Like all
BASIC statements, `REM` statements must be numbered.

### Keywords

Keywords like `PRINT` and `FOR` can be entered in upper or lower
case. `?` is a synonym for print, e.g.:

    ? 2+2
    4

Note that "?", like "print" must be followed by whitepace. You can't
write 'print2+2' because `print2` is a valid variable name.

### Numbers

Numbers are decimal unless they begin with `#`, in which case they
are hexadecimal.

### Arithmetic

PMBASIC supports only signed integer arithmetic. The usual
`+`, `-`, `\*`, and `/` are supported, along with a modulo division (`%`). 
Bitwise AND and bitwise OR are indicated using `&` and `|`. PMBASIC
follows the usual rules of operator precedence. 

All arithmetic operations can overflow and wrap around, and no warning
is shown. This is an occupational hazard of working with integers.

### Numbers

Numbers are in decimal unless they are preceded by `#`, which indicates
a hexadecimal number. Hex numbers can be entered using upper-case
or lower-case letters for the digits A-F. 

### Variables

Any number of integer variables can be defined, with names of
length up to 40 characters. In the program, names are
<i>case sensitive</i> (although keywords are not).

In a program, you can assign a variable by writing

    let a=2

or just 

    a=2
 
At the prompt, however, the `let` is required.

In a break with tradition, _variables must be assigned before use_.

### IF ... THEN and comparisons 

The format is

    IF {test} THEN {statement} ELSE {statement} 

These statements cannot span multiple lines. `test` can be a simple
variable, where a zero represents 'false' and anything else 'true', or
it can be an expression. The supported comparisons are `>`, `<` and `=`.
Comparisons can be combined -- 

`if (count = 0) & (key = 27) then...`

However, the brackets are strongly advised, because the comparison 
operators have the same precedence as `&` and `|`. 

Alternatively, IF statements can be nested on one line:

   IF {test1} THEN IF {test2} THEN {statement}

Tests can be inverted using NOT, e.g.,

    IF NOT finished THEN GOTO loop1
 
### FOR loops

The format of the FOR loop is

    FOR {variable} = {start} to {end} 
      {statement}
      {statement}
    NEXT

NEXT statements cannot indicate a loop control variable --
it's not possible to jump from one loop to another. 
FOR statements can be tested, to a depth
set at compile time in `config.h`. 

The value of the loop variable after the loop is one larger than
the end value.

It's possible to jump out of the middle of a loop using `GOTO`, but
this causes a memory leak, as PMBASIC does not know that the
data associated with the loop is no longer required.

### PRINT statement

`PRINT` can be abbreviated to `?`. `PRINT` outputs its arguments
without spacing, unless they are separated with a comma. `PRINT` 
writes a newline at the end of its argument list, unless the line
ends with a ';'. A `PRINT` on its own just writes a newline.

### GOTO and GOSUB

`GOTO` and `GOSUB` taken an expression as arguments, so there is some
runtime control of where to jump to. `GOSUB` can be nested, to
a limit defined in `config.h`  

### PEEK and POKE

These can be used to read and set specific memory locations. You can't
set the program flash ROM this way (for better or worse), but you can
read and write memory-mapped I/O registers. 

The format is:

    PEEK {address}, {variable} 
    POKE {address}, {value} 

On the atmega32u4 MCU, we have the following memory mapped ports 
(which should be similar on other Arduinos):

    DDRB = #24
    PORTB = #25
    The "RX" LED is bit 0 of `PORTB`. 

These addresses can be used with PEEK and POKE, so we can implement a
blinky like this:

    10 poke #24,1
    20 poke #25,1
    30 delay 1000
    50 poke #25,0
    60 delay 1000
    70 goto 10

Of course, using DIGITALWRITE is more elegant (and more portable)

### DELAY

`DELAY {expression>}` generates a millisecond delay. When compiled
in 16-bit mode, the longest delay is about 30 seconds. In 32-bit mode,
it is days.

A delay can't be interrupted using `ctrl+c`, because the logic needed to
work out if a serial character had been received would make it impossible
to get accurate delay timing. Of course, a `ctrl+c` will still take effect
when the delay is finished. Delay can be used in loops to get longer
times. The delay itself is pretty accurate, but the loop logic 
will add a few milliseconds to each loop.

### PINMODE

Sets a digital pin as input (0), output (1), or input-with-pullup (2).
The format is:

    PINMODE {pin}, {mode}

{pin} and {mode} can be expressions.

### DIGITALWRITE 

Sets an output pin to high (1) or low (0)

    DIGITALWRITE {pin}, {value}

{pin} and {value} can be expressions

### DIGITALREAD 

Reads a pin's state into a variable

    DIGITALREAD {pin}, {variable}

{pin} and {value} can be expressions. Note that the arguments to 
`PINMODE`, `DIGITALWRITE` and `DIGITALREAD` are just passed through to the 
C library functions `pinMode()`, `digitalRead()`, and `digitalWrite()`,
and the interpretation of the values is board-specific. Still, it's
more portable than POKEing `PORTB` directly, which also works.

### ANALOGWRITE

Sets an analog (PWM pin) to a specific analog level

    ANALOGWRITE {pin}, {value}

Not all pins are capable of PWM operation. On the Pro Micro, you can use 
pins 4, 6, 8, 9, 10, 18-21. These pins have different PWM frequencies
and, if you want to change them, you'll need to poke around with the
control registers -- see the datasheet for information.

In general, analog pins are set to a level from 0-255, that is, they
have 8-bit precision. 

### ANALOGREAD 

Reads an analog pin's state into a variable

    ANALOGREAD {pin}, {variable}

{pin} and {value} can be expressions. 

In general, analog inputs provide values in the range 0-1023.

### ANALOGREAD

### MILLIS 

Gets the time in milliseconds since PMBASIC stated. When compiled in
16-bit mode, this value will wrap around every 30 seconds or so. In
32-bit mode, this will take days.

    MILLIS {variable}


### INPUT

Prompt the user for a number, and store it in a variable

    INPUT a

The program will stop if you enter something that is not a number, or
hit ctrl+c during the input. It's not possible (yet) to enter a number
other than in decimal.

## Using the editor 

The editor is line-based, and similar to that provided by teletype basics of
the 70s.  PMBASIC assumes that any line entered that begins with a number is a
program statement to be stored, rather than executed. Conventionally, line
numbers are assigned in multiples of 10 or more, so you can insert a new line
between two existing ones, by choosing an intermediate number. 

Once a line has been stored, it can't be edited. However, it can be
replaced by entering a new line with the same number. If you enter
a number on its own, this deletes any existing line with the same
number.  

The editor automatically sorts lines by number, so you don't need
to enter them in order.

The only editing provided as text is entered is the "backspace" 
operation. PMBASIC echos characters received from the terminal
back to the terminal, but terminals vary in how they display
a backspace. In addition, some terminals send a DEL when the 
backspace key is pressed.  In such cases, pressing `ctrl+h` might generate
a real backspace. In the end, though, I can't really comment on terminal
configuration. Edit `arduino_interface.c` to change this, and other,
terminal codes, if you can't make your terminal emulator
do the right thing. It might be easier to reconfigure PMBASIC
than the terminal.

## Commands and immediate mode

Any input that starts with a letter is either a command or an immediate
statement. Not all BASIC statements can meaningfully be entered
in immediate mode. You can enter

    > PRINT 2+2

but not 

    > GOTO 50 

This is because an immediate statement is treated as a program, 
temporarily replacing the stored program. So statements that refer
to a stored program will not behave properly. IF ... THEN statements
can be used in immediate mode, although there's little need to.
However, a statement that spans multiple lines can't be used in 
immediate mode.

The following commands are available.

### NEW
 
Clear the program. There are no prompts or warnings. NEW doesn't
clear a program stored in EEPROM, nor does it clear global variables
that have been assigned values.

### CLEAR

Clear variables and reclaim any memory they used.

### LIST

List some or all of the program. The format is

    LIST [from [count]]

LIST on its own dumps the whole program.

### INFO 

Shows general information including memory usage.

### RUN

Runs the stored program. 

## SAVE

Save the current program into EEPROM. EEPROM access is slow-ish, and 
this could take a few seconds. It's plausible that a program that will
fit into RAM won't actually fit into EEPROM -- that depends on the
specific MCU chip. 

When PMBASIC saves to EEPROM, it writes the string "PMB" at the bottom
of the address range. This is because other programs use EEPROM, and
in radically different ways. If the "PMB" signature is present, that 
indicates that a program was saved, at least at some point. It's not
a foolproof way to prevent loading broken EEPROM data into RAM, but
it's better than nothing.

## LOAD

Loads a stored program from EEPROM. There stored program replaces any
existing program, without warning. PMBASIC can check that it stored a
program at some point in the past, but it can't be sure that the EEPROM
hasn't been fiddled with since then. Loading corrupt EEPROM data will
break PMBASIC, but there's not much that can be done about this.

## QUIT

Exits the PMBASIC programmer which, on a microcontroller, will cause
it to halt.

## Running a program

You can run the stored program by executing

    > RUN

Execution always starts from the lowest-numbered line.

## Stopping a program (and stopping other things)

You can interrupt a running program by sending `ctrl+c` from the
terminal (this value can be edited in `arduino_interface.c`).

Note that PMBASIC only checks whether `ctrl+c` has been pressed
at the start of each statement. A `DELAY`, for example, cannot
be interrupted.

You can also interrupt an INPUT statement the same way. If
you press `ctrl+c` in the editor, the line is discarded.

## Terminal issues

PMBASIC is designed to be used with a terminal emulator (Minicom, PuTTY...)
or perhaps even a real "dumb" terminal, if you can find one with a USB
connection (or, frankly, find one at all these days). Terminals are
notoriously fussy about certain things, such as what code corresponds
to the backspace key, and whether or not a line-feed should be 
followed by a carriage-return. 

The settings used by PMBASIC are correct for use with the Minicom 
terminal emulator, on Linux, in its default configuration.
However, there are a number of terminal settings defined in
`arduino-interface.h` that allow the terminal behaviour to be
tweaked, if necessary.

## Building

PMBasic is designed to be built using `make` and command-line tools on
Linux. I suspect you could take the C, C++, and header files and graft
them into a 'sketch' for the Arduino IDE, but I don't do IDEs so I don't
really know. The program _does_ use the Arduino official libraries, 
however, which are perhaps easiest to obtain along with the IDE. 
It will almost certainly be necessary to hack on the `Makefile` to
indicate the locations of the library sources. Arduino being what it is,
the libraries have to be compiled from source for each board, so
the Makefile is intended to take care of this along with building the
program. 

If everything is setup properly, running

    $ make -f Makefile.promicro

should give you a `.hex` file, ready for uploading to the board using
`avrdude` or whatever.  

For testing purposes, you can build a Linux console version by doing

    $ make -f Makefile.linux

The Linux version is intended to have exactly the same limited functionality
as the Arduino build.

## Technical details

### Parser

The parser used by PMBASIC is very simple -- but it's processing a very
simple language. Every line has the same basic structure, and the 
language can essentially be parsed on a line-by-line basis. 

Consequently, the parser is basically a pattern-matcher, with a
recursive-decent element for parsing arithmetic expressions.

The tokenizer and parser and parser are implemented in 
`tokenizer.c` and `parser.c` respectively. However, the operations
of the tokenzer and parser are tightly coupled, as are parsing
and execution. It's all a bit ugly, but the ugliness is hard to
avoid when we're working in an environment with such meagre resources.

### Grammar

Here is a description of PMBASIC's grammar. For ease of interpretation,
all branches in the syntax tree have a corresponding function in 
`basic.c`, with a name beginning `node_`. 

    program <-- (line_statement)*

    line_statement <-- [number] statement [eol]

    statement <--
      print_statement
      if_statement
      goto_statement
      gosub_statement
      return_statement
      for_statement
      peek_statement
      poke_statement
      next_statement
      end_statement
      let_statement
      rem_statement
      delay_statement 
      digitalwrite_statement
      digitalread_statement
      pinmode_statement
      input_statement
      millis_statement

      print_statement <-- PRINT ( [string] | [comma] | [semicolon] | expr  )*

      if_statement <-- IF relation statement ELSE statement 

      goto_statement <-- GOTO expr 

      gosub_statement <-- GOSUB expr 

      return_statement <-- RETURN

      for_statement <-- FOR <variable> '=' expr TO expr (line_statement [eol])* NEXT [variable]

      relation <-- expr ( '<' | '>' | '=' ) expr

      expr <-- term ( '+' | '-' | '&' | '|' | term )*

      term <-- factor ( '*' | '/' | '%' | factor)*

      factor <-- (-)* [number] | '(' expr ')' | varfactor

      varfactor <-- [variable]

      rem_statement <-- (?)* [eol] 

      let_statement <-- (LET)* <variable> = expression
      
      peek_statement <-- PEEK expr ',' <variable>

      poke_statement <-- POKE expr ',' expression

      digitalread_statement <-- DIGITALREAD expr ',' <variable>

      digitalwrite_statement <-- DIGITALWRITE expr ',' expression

      next_statement <-- NEXT [variable]

      end_statement <-- END
      
      delay_statement <-- DELAY expression

      input_statement <-- INPUT [variable]

      millis_statement <-- MILLIS [variable]

### Memory management issues

Memory management represents the biggest challenge to implementing
a useful -- even minimally useful -- BASIC environment on a Pro
Micro or similar. Of the 2kB or so of available RAM, we end up with
about 1kB free for program code -- and it's not easy to keep 
even that much free. 

The problem is that the AVR architecture makes a sharp distinction
between program and data memory. You can store data in the program
memory (flash), but the MCU can't perform any substantive operations
on it. All it can do is copy the stored data to RAM for processing.
Similarly, you can't store executable code in RAM.

The operation of a programming language parser centres on text 
processing. Most obviously, it has to compare the text of the
program, with a set of language keywords and tokens. 
It also needs to be able
to produce error messages that are at least long enough to be
comprehensible. All this data can be stored in flash, which has
ample capacity for a simple BASIC interpreter -- but it has to be
copied to RAM to be used.

In normal Arduino C programming, initialized data like this:

     char *keywords[] = {"PRINT", "IF", "THEN",...} 

is copied to RAM when the program starts.  
Thereafter it can be manipulated directly. But the total amount of
text data used by PMBASIC will more-or-less fill the RAM on its own.
Consequently, we have to retain this data in flash, and copy it
line-by-line into RAM when it's required. That's relatively easy to
do with large blocks of text like error messages that are used
infrequently, but it's very fiddly with small pieces of frequently-used
data, like program keywords.

These problems can be overcome, with some effort. But, in the end,
there's probably no point. There's no place to store program code
except in the EEPROM, and the Pro Micro on has 1kB of that. It's not
clear to me that there's much to be gained by extending the PMBASIC
language beyond its current, proof-of-concept, design. 
 
However, PMBASIC only occupries about 20kB of the 32kB flash in the
Pro Micro, and it would be easy to extend it with more Arduino-specific
capabilities. It would be easy, for example, to add code to operate
an LCD display, or play audio tones using PWM.


