// Test to verify that _SBCS being defined causes CharacterSet to be set to 0
// (Single Byte Character Set)

int main()
{
#ifdef _UNICODE
  bool UnicodeSet = true;
#else
  bool UnicodeSet = false;
#endif

#ifdef _MBCS
  bool MBCSSet = true;
#else
  bool MBCSSet = false;
#endif

  // if neither _UNICODE nor _MBCS is set, CharacterSet must be set to SBCS.
  bool SBCSSet = (!UnicodeSet && !MBCSSet);

  // Reverse boolean to indicate error case correctly
  return !SBCSSet;
}
