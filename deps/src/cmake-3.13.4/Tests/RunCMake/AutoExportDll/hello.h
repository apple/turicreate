#ifndef _MSC_VER
#  define winexport
#else
#  ifdef autoexport_EXPORTS
#    define winexport
#  else
#    define winexport __declspec(dllimport)
#  endif
#endif

class Hello
{
public:
  static winexport int Data;
  void real();
  static void operator delete[](void*);
  static void operator delete(void*);
};
