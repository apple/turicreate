using System;
using CSharpLibrary;

namespace CSharpLinkFromCxx
{
    internal class CSharpLinkFromCxx
    {
        public static void Main(string[] args)
        {
            Console.WriteLine("Starting test for CSharpLinkFromCxx");

            var useful = new UsefulManagedCppClass();
            useful.RunTest();
        }
    }
}
