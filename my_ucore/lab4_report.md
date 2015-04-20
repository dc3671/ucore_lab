# Lab4 report #

[TOC]

## 练习1 ##

[练习1.1]请在实验报告中简要说明你的设计实现过程。

- 明白这里是先用kmalloc给TCB分配内存空间，再初始化TCB，所以依次对`proc_struct`的变量进行初始化即可。state可以根据其枚举类型找到有指代未初始化的`PROC_UNINIT`，pid由于正常值是大等于0，所以设为-1好了，cr3的值比较难找，不过查看pmm_init()这个初始分配内存的函数之后可以发现`boot_cr3 = PADDR(boot_pgdir);`所以cr3也应该设置为这个内核空间的基址。

**与参考答案的不同:**

- 基本一致

[练习1.2]请说明proc_struct中struct context context和struct trapframe *tf成员变量含义和在本实验中的作用是啥？（提示通过看代码和编程调试可以判断出来）

- context顾名思义为进程的上下文，其中的成员变量是8个寄存器的值，在switch.S中通过两个进程的context结构进行寄存器的保存和切换。
- tf即trapframe，表示进程的中断帧结构，用于在中断时保存中断前的运行状态，段寄存器、堆栈指针、错误代码等。

## 练习2 ##

[练习2.1]请在实验报告中简要说明你的设计实现过程。

- 根据注释的提示：
- 首先调用`alloc_proc()`为新进程分配一个TCB，然后根据当前状态设置父进程TCB的指针为current当前进程，再调用`setup_kstack()`分配栈空间，再调用`copy_mm()`复制地址空间（lab4这里并未实际进行复制操作），
- 然后将进程的执行状态用`copy_thread()`拷贝到新的栈与地址空间中，设置子进程独立的返回值和上下文（`context.eip`指代中断前执行的指令，`context.esp`指代中断前栈顶指针）。
- 分配并设置新线程的pid，将其TCB加入到`hash_list`和`proc_list`中，调用`wakeup_proc`将进程唤醒准备执行。

**与参考答案的不同:**

- 基本一致。

[练习2.2]请说明ucore是否做到给每个新fork的线程一个唯一的id？请说明你的分析和理由。

- 是，确定是唯一的。
- 在`get_pid()`函数里，会遍历`proc_list`队列，函数返回的唯一途径为while循环遍历链表过程中没有发现`last_pid`与链表中已有进程的pid相同。如果有相同都会重新设置pid，`next_safe`记录了遍历过的所有进程pid中的最小值，`last_pid`在超过`next_safe`前可以直接自加来得到一个与之前遍历过的进程pid和当前遍历的进程pid都不相同的pid，而不用从头再进行遍历，只有当`last_pid`超过MAX_PID时才会从头开始再遍历。
- 总之，这样下来便可以快速得到一个与其他进程都不相同的pid。

## 练习3 ##

[练习3.1]请在实验报告中简要说明你对proc_run函数的分析。

- 通过查看proc_run()的代码，可以发现核心三条语句如下：

```
load_esp0(next->kstack + KSTACKSIZE);
lcr3(next->cr3);
switch_to(&(prev->context), &(next->context));
```

- 第一条语句修改TSS任务状态栈，将TSS的`ts_esp0`(stack pointers and segment selectors)指向下一个进程的堆栈空间。
- 第二条语句修改cr3，即页表基址。
- 第三条语句进行切换，这里通过gdb跟踪调试可以看到具体的跳转过程，先在switch.S里保存上下文到current->context，再把next->context里的变量值赋给寄存器，完成上下文切换。然后ret。
- 按理说本来调用`proc_run()`，返回也应该返回到这个函数里去，但是由于上下文切换，我们就返回到第二个参数也就是新进程里设置好的上下文中指定的地方去了，即在`copy_thread()`里将`context的eip`变量设为的`(uintptr_t)forkret`。这个函数又会跳到trapentry.S文件里的`forkrets`，设置好中断栈帧，再跳到`__trapret`，进行一系列中断完成前的准备，包括清空栈帧，执行中断前的指令： 

```
addl $0x8, %esp
iret
```

跳转到entry.S的5、6行：

```
pushl %edx              # push arg
call *%ebx              # call fn
```

从而进入`init_main()`函数，开始执行新的init线程。

[练习3.2]在本实验的执行过程中，创建且运行了几个内核线程？

- 总共两个，一个idleproc，一个initproc。idleproc在`proc_init()`中直接创建，而initproc则是由idleproc通过`do_fork()`得到。

[练习3.2]语句`local_intr_save(intr_flag);`....`local_intr_restore(intr_flag);`在这里有何作用?请说明理由

- 用于关闭中断和恢复中断，调用汇编指令sti和cli实现。
- STI(Set Interrupt) 中断标志置1指令 使 IF = 1；
- CLI(Clear Interrupt) 中断标志置0指令 使 IF = 0.

## 重要知识点 ##

- 实验中对进程调度时的PCB结构描述的比较清楚，从单进程运行到多进程调度的实现过程也较为明晰，主要体现在idleproc和initproc的创建。
- 原理课中叙述的进程的各种状态以及进程的控制功能在实验中没有实现。
