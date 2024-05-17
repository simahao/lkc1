# CRAY Linux
## 本地运行

`make local`

```makefile
local:
	@make build platform=qemu
	@make fs
	@$(QEMU) $(QEMUOPTS)
```

local目标中，要做三件工作。

1. 编译内核
2. 生成sdcard.img，将用户程序、测试用例等copy到sdcard.img中测试用例
3. 使用qemu-system-riscv64命令启动虚拟机，模拟评测流程

## 评测流程

1. 通过`make all`命令生成kernel-qemu内核文件
2. 评测机会调用如下命令自动启动qemu模拟器

```shell
qemu-system-riscv64 -machine virt -kernel kernel-qemu -m 128M -nographic -smp 2 -bios default -drive file=sdcard.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -no-reboot -device virtio-net-device,netdev=net -netdev user,id=net
```

3. kernel-qemu的内核需要自动扫描评测机的sdcard.img所对应的镜像文件，镜像文件根目录下存放32个测试程序
4. 根据自己内核的实现情况，在内核进入用户态后主动调用这些测试程序
5. 调用完测试程序后，主动关机退出

## 模拟评测系统的sdcard.img

为了能够在本地模拟这个评测过程，我们需要自己生成sdcard.img镜像，并且将测试程序放置到镜像的根目录下

```makefile
fs: $(UPROGS)
    @if [ ! -f "sdcard.img" ]; then \
        echo "making fs image..."; \
        #如果sdcard.img不存在，就创建，执行make clean后会删除这个文件
        dd if=/dev/zero of=sdcard.img bs=1M count=128; \
        mkfs.vfat -F 32 sdcard.img; fi
    #将sdcard.img挂载到/mnt目录，方便将测试用例copy到sdcard.img中
    @mount sdcard.img $(dst)
    @if [ ! -d "$(dst)/bin" ]; then mkdir $(dst)/bin; fi
    #将用户程序copy到sdcard.img中，注意，这里的用户程序不是测试用例，是ls/echo等
    @for file in $$( ls $U/_* ); do \
        cp $$file $(dst)/bin/$${file#$U/_}; done
    #这个地方，我们改造了一下，将测试用例通过tests目录存放好，在这里直接copy到sdcard.img中
    @cp -R tests/* $(dst)
    @umount $(dst)
```

## 如何自动调用评测系统的测试用例

操作系统内核初始化结束后，会进入用户态模式，比如说调用`sh`程序，进入交互式模式。因此我们需要完成两项主要的工作。

1. 生成一个类似于shell的用户程序(init)，这个程序主要完成调用sdcard.img中测试用例

```c

//因为用户程序执行系统调用，需要riscv下的这个函数，syscall0,0代表系统调用没有参数，我们这里添加这个函数主要目的是支持shutdown调用
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
// 这里放置了支持的系统调用
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

};

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
      wpid = wait((int *)0);
      if (wpid == pid) {
        break;
      }
      else if (wpid < 0) {
        printf("init: wait returned an error\n");
        exit(1);
      }
      else {
      }
    }
  }
  __syscall0(SYS_shutdown);
  return 0;
}


```

2. 将init的十六进形式加载到操作系统内核对应的内存中，并运行。为了提高效率，将init生成16进制代码的过程，形成了shell脚本。在makefile中，我们添加了test目标，test目标会调用runtest.sh脚本。

```shell
riscv64-linux-gnu-objcopy -S -O binary xv6-user/_init oo
od -v -t x1 -An oo | sed -E 's/ (.{2})/0x\1,/g' > kernel/include/initcode.h
rm oo
```

其中`riscv64-linux-gnu-objcopy -S -O binary xv6-user/_init oo`是将gcc编译好的_init程序，通过objcopy命令，保存符号表，并且以二进制的方式生成oo文件。`od -v -t x1 -An oo`命令将二进制文件，生成16进制，生成过程中去除地址，并且不要使用'*'来标注重复的信息。`sed -E 's/ (.{2})/0x\1,/g' > kernel/include/initcode.h`命令将每一个十六进制的数据前面添加`0x`前缀，字段间通过逗号间隔，然后将结果输出到`kernel/include/initcode.h`文件中。

initcode.h代码片段如下

```c
0x5d,0x71,0xa2,0xe0,0x86,0xe4,0x26,0xfc,0x4a,0xf8,0x4e,0xf4,0x52,0xf0,0x56,0xec,
0x5a,0xe8,0x5e,0xe4,0x62,0xe0,0x80,0x08,0x93,0x05,0x60,0x1b,0x17,0x15,0x00,0x00,
0x13,0x05,0x45,0xdf,0x97,0x10,0x00,0x00,0xe7,0x80,0xe0,0xbb,0x09,0x46,0x97,0x15,
0x00,0x00,0x93,0x85,0xa5,0xde,0x13,0x05,0xc0,0xf9,0x97,0x10,0x00,0x00,0xe7,0x80,
0x80,0x84,0x63,0x44,0x05,0x10,0x01,0x45,0x97,0x10,0x00,0x00,0xe7,0x80,0xc0,0xbe,
0x01,0x45,0x97,0x10,0x00,0x00,0xe7,0x80,0x20,0xbe,0x17,0x2a,0x00,0x00,0x13,0x0a,
0x6a,0xfa,0x83,0x25,0x0a,0x00,0x17,0x15,0x00,0x00,0x13,0x05,0x25,0xdc,0x97,0x00,
0x00,0x00,0xe7,0x80,0xa0,0x7b,0x83,0x27,0x0a,0x00,0x63,0x51,0xf0,0x08,0x97,0x29,
0x00,0x00,0x93,0x89,0x29,0xf9,0x01,0x49,0x97,0x2b,0x00,0x00,0x93,0x8b,0x0b,0x08,
0x17,0x1b,0x00,0x00,0x13,0x0b,0x0b,0xdd,0x17,0x1c,0x00,0x00,0x13,0x0c,0x0c,0xdb,
```

这段代码通过内核初始化最后的阶段进行加载，代码如下

```c
uchar initcode[] = {
#include "include/initcode.h"
};
// Set up first user process.
void userinit(void) {
  struct proc *p;
  p = allocproc();
  initproc = p;
  // allocate one user page and copy init's instructions
  // and data into it.
  uvminit(p->pagetable , p->kpagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;
  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0x0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer
  safestrcpy(p->name, "init", sizeof(p->name));
  p->state = RUNNABLE;
  p->tmask = 0;
  release(&p->lock);
  #ifdef DEBUG
  printf("userinit\n");
  #endif
}
```