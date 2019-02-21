
extern __device__ int file1_func(int);
int __device__ file3_func(int x)
{
  if (x > 0)
    return file1_func(-x);
  return x;
}
