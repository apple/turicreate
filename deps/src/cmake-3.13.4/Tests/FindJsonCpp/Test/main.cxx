#include <json/json.h>

int main()
{
  int zero = 0;
  Json::Value value(zero);
  return value.asInt();
}
