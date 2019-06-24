#ifndef TURI_STYLE_TRANSFER_UTILS_H_
#define TURI_STYLE_TRANSFER_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

struct weight {
  char * name;
  float* data;
  int count;
};

struct weights {
  struct weight* data;
  int count;
};

#ifdef __cplusplus
}
#endif

#endif