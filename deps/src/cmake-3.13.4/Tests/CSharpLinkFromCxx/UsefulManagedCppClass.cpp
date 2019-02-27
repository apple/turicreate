#include "UsefulManagedCppClass.hpp"

namespace CSharpLibrary
{
    UsefulManagedCppClass::UsefulManagedCppClass()
    {
        auto useful = gcnew UsefulCSharpClass();
        m_usefulString = useful->GetSomethingUseful();
    }

    void UsefulManagedCppClass::RunTest()
    {
        Console::WriteLine("Printing from Managed CPP Class: " + m_usefulString);
    }
}
