#ifndef TURI_CAPI_INITIALIZATION_INTERNAL
#define TURI_CAPI_INITIALIZATION_INTERNAL

namespace turi {
void _tc_initialize();

extern bool capi_server_initialized;

static inline void ensure_server_initialized() {
  if (!capi_server_initialized) {
    _tc_initialize();
  }
}

}  // namespace turi
#endif
