
import foo
import unittest

class TestFoo(unittest.TestCase):

    def testFoo(self):
        self.assertEquals(foo.foo(), 6, 'foo() == 6')

if __name__ == '__main__':
    unittest.main()
