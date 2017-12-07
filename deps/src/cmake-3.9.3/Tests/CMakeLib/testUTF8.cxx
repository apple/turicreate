/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include <cm_utf8.h>
#include <stdio.h>

typedef char test_utf8_char[5];

static void test_utf8_char_print(test_utf8_char const c)
{
  unsigned char const* d = reinterpret_cast<unsigned char const*>(c);
  printf("[0x%02X,0x%02X,0x%02X,0x%02X]", (int)d[0], (int)d[1], (int)d[2],
         (int)d[3]);
}

struct test_utf8_entry
{
  int n;
  test_utf8_char str;
  unsigned int chr;
};

static test_utf8_entry const good_entry[] = {
  { 1, "\x20\x00\x00\x00", 0x0020 },  /* Space.  */
  { 2, "\xC2\xA9\x00\x00", 0x00A9 },  /* Copyright.  */
  { 3, "\xE2\x80\x98\x00", 0x2018 },  /* Open-single-quote.  */
  { 3, "\xE2\x80\x99\x00", 0x2019 },  /* Close-single-quote.  */
  { 4, "\xF0\xA3\x8E\xB4", 0x233B4 }, /* Example from RFC 3629.  */
  { 0, { 0, 0, 0, 0, 0 }, 0 }
};

static test_utf8_char const bad_chars[] = {
  "\x80\x00\x00\x00", "\xC0\x00\x00\x00", "\xE0\x00\x00\x00",
  "\xE0\x80\x80\x00", "\xF0\x80\x80\x80", { 0, 0, 0, 0, 0 }
};

static void report_good(bool passed, test_utf8_char const c)
{
  printf("%s: decoding good ", passed ? "pass" : "FAIL");
  test_utf8_char_print(c);
  printf(" (%s) ", c);
}

static void report_bad(bool passed, test_utf8_char const c)
{
  printf("%s: decoding bad  ", passed ? "pass" : "FAIL");
  test_utf8_char_print(c);
  printf(" ");
}

static bool decode_good(test_utf8_entry const entry)
{
  unsigned int uc;
  if (const char* e =
        cm_utf8_decode_character(entry.str, entry.str + 4, &uc)) {
    int used = static_cast<int>(e - entry.str);
    if (uc != entry.chr) {
      report_good(false, entry.str);
      printf("expected 0x%04X, got 0x%04X\n", entry.chr, uc);
      return false;
    }
    if (used != entry.n) {
      report_good(false, entry.str);
      printf("had %d bytes, used %d\n", entry.n, used);
      return false;
    }
    report_good(true, entry.str);
    printf("got 0x%04X\n", uc);
    return true;
  }
  report_good(false, entry.str);
  printf("failed\n");
  return false;
}

static bool decode_bad(test_utf8_char const s)
{
  unsigned int uc = 0xFFFFu;
  const char* e = cm_utf8_decode_character(s, s + 4, &uc);
  if (e) {
    report_bad(false, s);
    printf("expected failure, got 0x%04X\n", uc);
    return false;
  }
  report_bad(true, s);
  printf("failed as expected\n");
  return true;
}

int testUTF8(int /*unused*/, char* /*unused*/ [])
{
  int result = 0;
  for (test_utf8_entry const* e = good_entry; e->n; ++e) {
    if (!decode_good(*e)) {
      result = 1;
    }
  }
  for (test_utf8_char const* c = bad_chars; (*c)[0]; ++c) {
    if (!decode_bad(*c)) {
      result = 1;
    }
  }
  return result;
}
