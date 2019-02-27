#include "uv.h"
#include "internal.h"

int uv__tcp_nodelay(int fd, int on) {
  errno = EINVAL;
  return -1;
}

int uv__tcp_keepalive(int fd, int on, unsigned int delay) {
  errno = EINVAL;
  return -1;
}

int uv_tcp_listen(uv_tcp_t* tcp, int backlog, uv_connection_cb cb) {
  return -EINVAL;
}

int uv_udp_open(uv_udp_t* handle, uv_os_sock_t sock) {
  return -EINVAL;
}

void uv__tcp_close(uv_tcp_t* handle) {
}

void uv__udp_close(uv_udp_t* handle) {
}

void uv__udp_finish_close(uv_udp_t* handle) {
}

void uv__fs_poll_close(uv_fs_poll_t* handle) {
}

int uv_async_init(uv_loop_t* loop, uv_async_t* handle, uv_async_cb async_cb) {
  return 0;
}

void uv__async_close(uv_async_t* handle) {
}

int uv__async_fork(uv_loop_t* loop) {
  return 0;
}

void uv__async_stop(uv_loop_t* loop) {
}

void uv__work_submit(uv_loop_t* loop, struct uv__work* w,
                     void (*work)(struct uv__work* w),
                     void (*done)(struct uv__work* w, int status)) {
  abort();
}

void uv__work_done(uv_async_t* handle) {
}

int uv__pthread_atfork(void (*prepare)(void), void (*parent)(void),
                       void (*child)(void)) {
  return 0;
}

int uv__pthread_sigmask(int how, const sigset_t* set, sigset_t* oset) {
  return 0;
}

int uv_mutex_init(uv_mutex_t* mutex) {
  return 0;
}

void uv_mutex_destroy(uv_mutex_t* mutex) {
}

void uv_mutex_lock(uv_mutex_t* mutex) {
}

void uv_mutex_unlock(uv_mutex_t* mutex) {
}

int uv_rwlock_init(uv_rwlock_t* rwlock) {
  return 0;
}

void uv_rwlock_destroy(uv_rwlock_t* rwlock) {
}

void uv_rwlock_wrlock(uv_rwlock_t* rwlock) {
}

void uv_rwlock_wrunlock(uv_rwlock_t* rwlock) {
}

void uv_rwlock_rdlock(uv_rwlock_t* rwlock) {
}

void uv_rwlock_rdunlock(uv_rwlock_t* rwlock) {
}

void uv_once(uv_once_t* guard, void (*callback)(void)) {
  if (*guard) {
    return;
  }
  *guard = 1;
  callback();
}

#if defined(__linux__)
int uv__accept4(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) {
  errno = ENOSYS;
  return -1;
}

int uv__dup3(int oldfd, int newfd, int flags) {
  errno = ENOSYS;
  return -1;
}

int uv__pipe2(int pipefd[2], int flags) {
  errno = ENOSYS;
  return -1;
}

ssize_t uv__preadv(int fd, const struct iovec *iov, int iovcnt,
                   int64_t offset) {
  errno = ENOSYS;
  return -1;
}

ssize_t uv__pwritev(int fd, const struct iovec *iov, int iovcnt,
                    int64_t offset) {
  errno = ENOSYS;
  return -1;
}

int uv__utimesat(int dirfd, const char* path, const struct timespec times[2],
                 int flags) {
  errno = ENOSYS;
  return -1;
}
#endif
