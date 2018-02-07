import unittest
import six

class TestCase(unittest.TestCase):
    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName)
        import turicreate as tc
        # DEPRECATED: tc.canvas.set_target('headless')

    # override assertEqual and assertNotEqual,
    # since they are not type safe in Python.
    # non type-safe comparisons pass when they should fail, a lot.
    def assertEqual(self, a, b):
        both_int_types = isinstance(a, six.integer_types) and isinstance(b, six.integer_types)
        if not both_int_types:
            super(TestCase, self).assertEqual(type(a), type(b))
        super(TestCase, self).assertEqual(a, b)

    def assertNotEqual(self, a, b):
        both_int_types = isinstance(a, six.integer_types) and isinstance(b, six.integer_types)
        if not both_int_types:
            super(TestCase, self).assertEqual(type(a), type(b))
        super(TestCase, self).assertNotEqual(a, b)

    # useful methods for our own purposes
    def assertWithinRange(self, value, target, allowed_range):
        # asserts that value is within allowed_range of target
        self.assertEqual(max(abs(value - target), allowed_range), allowed_range)

    def assertBetween(self, value, lower, upper):
        # asserts that a value is between two bounds, including the bounds
        self.assertGreaterEqual(value, lower)
        self.assertLessEqual(value, upper)


def check_output(command_args):
    """ Wrapper for subprocess.checkoutput with logging of the error of the failed command """
    import subprocess
    import logging
    from subprocess import CalledProcessError
    try:
        output = subprocess.check_output(command_args, stderr=subprocess.STDOUT)[:-1]  # [:-1] removes the trailing \n
        return output
    except CalledProcessError as e:
        logging.warning("Failed call with Output: %s" % e.output)
        raise e

