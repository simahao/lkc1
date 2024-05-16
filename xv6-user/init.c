// init: The initial user-level program

#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "kernel/include/file.h"
#include "kernel/include/fcntl.h"
#include "kernel/include/sysnum.h"
#include "xv6-user/user.h"

#define __asm_syscall(...)             \
    __asm__ __volatile__("ecall\n\t"   \
                         : "=r"(a0)    \
                         : __VA_ARGS__ \
                         : "memory");  \
    return a0;

static inline long __syscall0(long n)
{
    register long a7 __asm__("a7") = n;
    register long a0 __asm__("a0");
    __asm_syscall("r"(a7))
}

char *argv[] = {0};
char *tests[] = {
    "brk",
    "chdir",
    "close",
    "dup",
    "exit",
    "fork",
    "fstat",
    "getcwd",
    "getpid",
    "gettimeofday",
    "mkdir_",
    "openat",
    "open",
    "pipe",
    "read",
    "uname",
    "wait",
    "write",
    "sleep",
    "clone",

    // "execve",
    // "getdents",
    // "getppid",
    // "mmap",
    // "mount",
    // "munmap",
    // "yield",
    // "waitpid",
    // "dup2",
    // "times",
    // "umount",
    // "unlink",
};
// char *tests[] = {
//     "chdir",
//     "close",
//     "dup",
//     "exit",
//     "fork",
//     "fstat",
//     "getpid",
//     "mkdir_",
//     "open",
//     "pipe",
//     "read",
//     "brk",
//     "sleep",
//     "wait",
//     "write",
//     "getcwd",
// };

int counts = sizeof(tests) / sizeof((tests)[0]);

int main(void) {
  int pid, wpid;
  dev(O_RDWR, CONSOLE, 0);
  dup(0); // stdout
  dup(0); // stderr

  for (int i = 0; i < counts; i++) {
    printf("init: starting sh\n");
    pid = fork();
    if (pid < 0) {
        printf("init: fork failed\n");
        exit(1);
    }
    if (pid == 0) {
        exec(tests[i], argv);
        printf("init: exec sh failed\n");
        exit(1);
    }

    for (;;) {
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *)0);
      if (wpid == pid) {
        // the shell exited; restart it.
        break;
      }
      else if (wpid < 0) {
        printf("init: wait returned an error\n");
        exit(1);
      }
      else {
        // it was a parentless process; do nothing.
      }
    }
  }
  __syscall0(SYS_shutdown);
  return 0;
}