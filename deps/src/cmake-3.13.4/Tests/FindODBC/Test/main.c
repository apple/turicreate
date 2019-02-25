#ifdef WIN32
#  include <windows.h>
#endif
#include <sql.h>

int main()
{
  SQLHENV env;
  SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);
  SQLFreeHandle(SQL_HANDLE_ENV, env);
  return 0;
}
