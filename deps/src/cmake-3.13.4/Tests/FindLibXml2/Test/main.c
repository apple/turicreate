#include <assert.h>
#include <libxml/tree.h>
#include <string.h>

int main()
{
  xmlDoc* doc;

  xmlInitParser();

  doc = xmlNewDoc(BAD_CAST "1.0");
  xmlFreeDoc(doc);

  assert(strstr(CMAKE_EXPECTED_LibXml2_VERSION, LIBXML_DOTTED_VERSION));

  xmlCleanupParser();

  return 0;
}
