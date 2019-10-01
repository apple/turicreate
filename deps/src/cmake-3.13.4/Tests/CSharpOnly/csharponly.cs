namespace CSharpOnly
{
    class CSharpOnly
    {
        public static void Main(string[] args)
        {
            int val = Lib1.getResult();

            Lib2 l = new Lib2();
            val = l.myVal;

            return;
        }
    }
}
