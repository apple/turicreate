
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#else
#define EXPORT
#define IMPORT
#endif

int dynamic_base_func(int);
IMPORT int cuda_dynamic_host_func(int);
int file3_launch_kernel(int);

int dynamic_final_func(int x)
{
  return cuda_dynamic_host_func(dynamic_base_func(x));
}

EXPORT int call_cuda_seperable_code(int x)
{
  return file3_launch_kernel(x);
}
