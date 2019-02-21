<?php

# This file illustrates the low-level C++ interface
# created by SWIG.  In this case, all of our C++ classes
# get converted into function calls.

require("example.php");

# ----- Object creation -----

print "Creating some objects:\n";
$c = new_Circle(10);
print "    Created circle $c\n";
$s = new_Square(10);
print "    Created square $s\n";

# ----- Access a static member -----

print "\nA total of " . nshapes() . " shapes were created\n";

# ----- Member data access -----

# Set the location of the object.
# Note: methods in the base class Shape are used since
# x and y are defined there.

Shape_x_set($c, 20);
Shape_y_set($c, 30);
Shape_x_set($s,-10);
Shape_y_set($s,5);

print "\nHere is their current position:\n";
print "    Circle = (" . Shape_x_get($c) . "," .  Shape_y_get($c) . ")\n";
print "    Square = (" . Shape_x_get($s) . "," .  Shape_y_get($s) . ")\n";

# ----- Call some methods -----

print "\nHere are some properties of the shapes:\n";
foreach (array($c,$s) as $o) {
      print "    $o\n";
      print "        area      = " .  Shape_area($o) .  "\n";
      print "        perimeter = " .  Shape_perimeter($o) . "\n";
  }
# Notice how the Shape_area() and Shape_perimeter() functions really
# invoke the appropriate virtual method on each object.

# ----- Delete everything -----

print "\nGuess I'll clean up now\n";

# Note: this invokes the virtual destructor
delete_Shape($c);
delete_Shape($s);

print nshapes() . " shapes remain\n";
print "Goodbye\n";

?>
