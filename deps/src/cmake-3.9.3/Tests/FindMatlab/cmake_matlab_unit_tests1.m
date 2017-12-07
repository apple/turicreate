
classdef cmake_matlab_unit_tests1 < matlab.unittest.TestCase
  % some simple unit test for CMake Matlab wrapper
  properties
  end

  methods (Test)
    function testDummyCall(testCase)
      % very simple call test
      cmake_matlab_mex1(rand(3,3));
    end

    function testDummyCall2(testCase)
      % very simple call test 2
      ret = cmake_matlab_mex1(rand(3,3));
      testCase.verifyEqual(size(ret), size(rand(3,3)));

      testCase.verifyEqual(size(cmake_matlab_mex1(rand(4,3))), [4,3] );
    end

    function testFailTest(testCase)
      testCase.verifyError(@() cmake_matlab_mex1(10), 'cmake_matlab:configuration');
      testCase.verifyError(@() cmake_matlab_mex1([10]), 'cmake_matlab:configuration');
    end

    function testHelpContent(testCase)
      % testing the help feature
      testCase.verifySubstring(evalc('help cmake_matlab_mex1'), 'Dummy matlab extension in cmake');
    end


  end
end
