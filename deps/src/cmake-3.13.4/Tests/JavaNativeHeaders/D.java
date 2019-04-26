class D
{
  public D()
    {
    }

    public native void printName();

    static {
        try {

            System.loadLibrary("D");

        } catch (UnsatisfiedLinkError e) {
            System.err.println("Native code library failed to load.\n" + e);
            System.exit(1);
        }
    }
}
