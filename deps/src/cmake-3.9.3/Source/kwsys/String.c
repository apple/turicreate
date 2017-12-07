/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing#kwsys for details.  */
#ifdef KWSYS_STRING_C
/*
All code in this source file is conditionally compiled to work-around
template definition auto-search on VMS.  Other source files in this
directory that use the stl string cause the compiler to load this
source to try to get the definition of the string template.  This
condition blocks the compiler from seeing the symbols defined here.
*/
#include "kwsysPrivate.h"
#include KWSYS_HEADER(String.h)

/* Work-around CMake dependency scanning limitation.  This must
   duplicate the above list of headers.  */
#if 0
#include "String.h.in"
#endif

/* Select an implementation for strcasecmp.  */
#if defined(_MSC_VER)
#define KWSYS_STRING_USE_STRICMP
#include <string.h>
#elif defined(__GNUC__)
#define KWSYS_STRING_USE_STRCASECMP
#include <strings.h>
#else
/* Table to convert upper case letters to lower case and leave all
   other characters alone.  */
static char kwsysString_strcasecmp_tolower[] = {
  '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007', '\010',
  '\011', '\012', '\013', '\014', '\015', '\016', '\017', '\020', '\021',
  '\022', '\023', '\024', '\025', '\026', '\027', '\030', '\031', '\032',
  '\033', '\034', '\035', '\036', '\037', '\040', '\041', '\042', '\043',
  '\044', '\045', '\046', '\047', '\050', '\051', '\052', '\053', '\054',
  '\055', '\056', '\057', '\060', '\061', '\062', '\063', '\064', '\065',
  '\066', '\067', '\070', '\071', '\072', '\073', '\074', '\075', '\076',
  '\077', '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157', '\160',
  '\161', '\162', '\163', '\164', '\165', '\166', '\167', '\170', '\171',
  '\172', '\133', '\134', '\135', '\136', '\137', '\140', '\141', '\142',
  '\143', '\144', '\145', '\146', '\147', '\150', '\151', '\152', '\153',
  '\154', '\155', '\156', '\157', '\160', '\161', '\162', '\163', '\164',
  '\165', '\166', '\167', '\170', '\171', '\172', '\173', '\174', '\175',
  '\176', '\177', '\200', '\201', '\202', '\203', '\204', '\205', '\206',
  '\207', '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
  '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227', '\230',
  '\231', '\232', '\233', '\234', '\235', '\236', '\237', '\240', '\241',
  '\242', '\243', '\244', '\245', '\246', '\247', '\250', '\251', '\252',
  '\253', '\254', '\255', '\256', '\257', '\260', '\261', '\262', '\263',
  '\264', '\265', '\266', '\267', '\270', '\271', '\272', '\273', '\274',
  '\275', '\276', '\277', '\300', '\301', '\302', '\303', '\304', '\305',
  '\306', '\307', '\310', '\311', '\312', '\313', '\314', '\315', '\316',
  '\317', '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
  '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337', '\340',
  '\341', '\342', '\343', '\344', '\345', '\346', '\347', '\350', '\351',
  '\352', '\353', '\354', '\355', '\356', '\357', '\360', '\361', '\362',
  '\363', '\364', '\365', '\366', '\367', '\370', '\371', '\372', '\373',
  '\374', '\375', '\376', '\377'
};
#endif

/*--------------------------------------------------------------------------*/
int kwsysString_strcasecmp(const char* lhs, const char* rhs)
{
#if defined(KWSYS_STRING_USE_STRICMP)
  return _stricmp(lhs, rhs);
#elif defined(KWSYS_STRING_USE_STRCASECMP)
  return strcasecmp(lhs, rhs);
#else
  const char* const lower = kwsysString_strcasecmp_tolower;
  unsigned char const* us1 = (unsigned char const*)lhs;
  unsigned char const* us2 = (unsigned char const*)rhs;
  int result;
  while ((result = lower[*us1] - lower[*us2++], result == 0) && *us1++) {
  }
  return result;
#endif
}

/*--------------------------------------------------------------------------*/
int kwsysString_strncasecmp(const char* lhs, const char* rhs, size_t n)
{
#if defined(KWSYS_STRING_USE_STRICMP)
  return _strnicmp(lhs, rhs, n);
#elif defined(KWSYS_STRING_USE_STRCASECMP)
  return strncasecmp(lhs, rhs, n);
#else
  const char* const lower = kwsysString_strcasecmp_tolower;
  unsigned char const* us1 = (unsigned char const*)lhs;
  unsigned char const* us2 = (unsigned char const*)rhs;
  int result = 0;
  while (n && (result = lower[*us1] - lower[*us2++], result == 0) && *us1++) {
    --n;
  }
  return result;
#endif
}

#endif /* KWSYS_STRING_C */
