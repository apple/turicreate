extern void NoFunction();

/* Provide a function that is supposed to be found in the Three
   library.  If Two links to TwoCustom then TwoCustom will come before
   Three and this symbol will be used.  Since NoFunction is not
   defined, that will cause a link failure.  */
void ThreeFunction()
{
  NoFunction();
}
