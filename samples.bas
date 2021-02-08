# Blinky with POKE 

10 poke #24,1
20 poke #25,1
30 delay 1000
40 poke #25,0
50 delay 1000
60 goto 10

# Blinky with SETPIN 

10 pin=17
20 pinmode pin,1
30 digitalwrite pin,1
40 delay 1000
50 digitalwrite pin,0
60 delay 1000
70 goto 10

# Times table

10 print "Enter a small number: ";
20 input a
30 for i=1 to a 
40    for j=1 to a
50      print i *j,;
60    next 
70 print
80 next 

# Loop timer

5 l = 100
10 millis a
20 for i = 1 to l
30   for j = 1 to l
40     rem
50   next
60 next
70 millis b
80 print l * l " loops in " b - a " msec"

# Pulsing LED (on pin 10, in this case)

10 pin = 10
20 for i = 0 to 64 
30   analogwrite pin,i
40   delay 10
50 next
60 for i = 0 to 64 
70   analogwrite pin,64-i
80   delay 10
90 next
100 goto 10





