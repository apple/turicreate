using System;
using CLIApp;

namespace CSharpLinkToCxx
{
  internal class CSharpLinkToCxx
  {
    public static void Main(string[] args)
    {
      Console.WriteLine("#message from CSharpLinkToCxx");

      var app = new MyCli();
      app.testMyCli();
    }
  }
}
