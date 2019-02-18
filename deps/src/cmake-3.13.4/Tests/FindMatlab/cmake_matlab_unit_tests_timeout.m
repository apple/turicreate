
classdef cmake_matlab_unit_tests_timeout < matlab.unittest.TestCase
  % timeout tests

  properties
  end

  methods (Test)
    function testCallHangsShouldBeTimedOut(testCase)
      cmake_matlab_mex1(rand(3,3));
      disp('Will now wait.');
      disp('Testing the cmake Matlab package timeout - do not kill');
      pause(20); % supposed to be killed after 15s
    end
  end
end
