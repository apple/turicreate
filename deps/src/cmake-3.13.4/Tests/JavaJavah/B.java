class B
{
  public B()
    {
    }

    public native void printName();

    static {
        try {

            System.loadLibrary("B");

        } catch (UnsatisfiedLinkError e) {
            System.err.println("Native code library failed to load.\n" + e);
            System.exit(1);
        }
    }
}
