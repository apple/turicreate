using System;

public class ReferenceImport
{
  public static void Main()
  {
    Console.WriteLine("ReferenceImportCSharp");
    ImportLibMixed.Message();
    ImportLibPure.Message();
    ImportLibSafe.Message();
    ImportLibCSharp.Message();
  }
}
