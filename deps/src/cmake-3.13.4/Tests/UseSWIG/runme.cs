using System;
using System.Collections.Generic;

public class runme
{
    static void Main()
    {
        // ----- Object creation -----

        Console.WriteLine("Creating some objects:");
        Circle c = new Circle(10);
        Console.WriteLine("    Created " + c);
        Square s = new Square(10);
        Console.WriteLine("    Created " + s);

        // ----- Access a static member -----

        Console.WriteLine("\nA total of " + Shape.nshapes + " shapes were created");

        // ----- Member data access -----

        // Set the location of the object

        c.x = 20;
        c.y = 30;

        s.x = -10;
        s.y = 5;

        Console.WriteLine("\nHere is their current position:");
        Console.WriteLine("    Circle = ({0}, {1})", c.x,c.y);
        Console.WriteLine("    Square = ({0}, {1})", s.x,s.y);

        // ----- Call some methods -----

        Console.WriteLine("\nHere are some properties of the shapes:");
        List <Shape> shapeList = new List <Shape> { c,s };
        foreach(var o in shapeList){
              Console.WriteLine("   " + o);
              Console.WriteLine("        area      = " + o.area());
              Console.WriteLine("        perimeter = " + o.perimeter());
        }

        Console.WriteLine("\nGuess I'll clean up now");

        // Note: this invokes the virtual destructor
        c.Dispose();
        s.Dispose();

        s = new Square(10);;
        Console.WriteLine(Shape.nshapes + " shapes remain");
        Console.WriteLine("Goodbye");
    }
}
