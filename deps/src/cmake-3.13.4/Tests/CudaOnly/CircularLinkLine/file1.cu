
extern __device__ int file2_func(int);
int __device__ file1_func(int x)
{
  return file2_func(x);
}
