
extern __device__ int file3_func(int);
int __device__ file2_func(int x)
{
  return x + file3_func(x);
}
