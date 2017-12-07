
ret = cmake_matlab_mex1(rand(3,3));

if(size(ret) ~= size(rand(3,3)))
  error('Dimension mismatch!');
end
