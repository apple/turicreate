# file: runme.py

# This file illustrates the shadow-class C++ interface generated
# by SWIG.

from __future__ import print_function

import example

# ----- Object creation -----

print ("Creating some objects:")
c = example.Circle(10)
print ("    Created circle", c)
s = example.Square(10)
print ("    Created square", s)

# ----- Access a static member -----

print ("\nA total of", example.cvar.Shape_nshapes,"shapes were created")

# ----- Member data access -----

# Set the location of the object

c.x = 20
c.y = 30

s.x = -10
s.y = 5

print ("\nHere is their current position:")
print ("    Circle = (%f, %f)" % (c.x,c.y))
print ("    Square = (%f, %f)" % (s.x,s.y))

# ----- Call some methods -----

print ("\nHere are some properties of the shapes:")
for o in [c,s]:
      print ("   ", o)
      print ("        area      = ", o.area())
      print ("        perimeter = ", o.perimeter())

print ("\nGuess I'll clean up now")

# Note: this invokes the virtual destructor
del c
del s

s = 3
print (example.cvar.Shape_nshapes,"shapes remain")
print ("Goodbye")
