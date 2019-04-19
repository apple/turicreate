#include "utils.hpp"

bool turi::annotate::isInteger(std::string s) {
  if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

  char * p;
  long long result = strtol(s.c_str(), &p, 10);
  (void) result; // silence compiler warning about unused return value

  return (*p == 0);
}
