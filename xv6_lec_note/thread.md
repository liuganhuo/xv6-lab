## thread



一个线程可以认为是一个串行执行的代码单元，只占用一个CPU。

其中PC，用户寄存器以及stack对其十分重要。



#### XV6中的线程

xv6中的内核线程机制最终是为了支持在多个用户进程之间进行切换。



##### xv6的三种thread

- 用户进程的user thread：上下文保存在trapframe中
  - 用户线程之间没有共享内存。
- 用户进程的kernel thread：上下文保存在trapframe和context中
  - 所有kernel thread共享内核内存。
- 每个CPU核的scheduler thread：上下文保存在context中

**xv6中线程与进程的关系**：process由2个thread组成。一个进程要么是其user thread 在运行，要么是在kernel thread运行(系统调用/响应中断)，要么不运行。



##### 信息记录：proc结构体

其中记录了

- **process本身的状态信息**，如pid , state , parent process , name等
- 和process中的**user thread**的所用的user pagetable , user的heap大小 , user的trapframe等
- 以及process中的**kernel thread 的context**等。(用于kernel thread 在swtch保存前一刻kernel thread的状态)

所以，通过mycpu()，我们可以获得当前正在运行的线程所属的process。不过从逻辑上来讲，我们也就不应当在scheduler thread中调用mycpu()。因为scheduler不属于任何process。是单独的一个线程。





#### XV6中线程调度

**xv6的线程调度策略**： pre-emptive scheduling(定时器中断，抢占式调度) + voluntary scheduling(kernel thread主动swtch)

内核会在如下场景下让出cpu

- 抢占式调度：利用定时器中断将CPU控制权给到内核，内核再自愿让出CPU
- 调用系统调用并等待I/O：如进程调用syscall read进入kernel，read会等待磁盘IO，此时read syscall code->sleep()->sched()->swtch()



##### thread调度流程

user thread A -> kernel thread A -> scheduler thread -> kernel thread B -> user thread B

<img src="https://cstardust.github.io/2022/11/18/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F-xv6-thread/2022-11-22-19-12-06.png" alt="img" style="zoom:50%;" />

- **kernel thread -> scheduler thread**：usertrap() -> yield() ->  sched() -> swtch() -> scheduler()
  - yield()：将当前进程设置为runnable
  - 在switch中 保存kernel thread context 并加载 scheduler thread context
- **scheduler thread -> kernel thread**：scheduler() -> swtch() -> sched() -> yield() -> 
  - 选出一个runnable进程，进行swtch()保存scheduler thread context 并加载另一个kernel thread context 
  - 并将当前进程设置为C->proc = 0，因为当前是CPU的线程。
  - **cheduler thread 调用了swtch函数，但是我们从swtch函数返回时，实际上是返回到了对于switch的另一个调用处：kenrel thread 对swtch的调用处**，而不是scheduler thread的调用。这样就设置好了myproc()，并返回到了kernel thread



##### thread切换的核心：swtch()

- 用于实现 kernel thread 和 scheduler thread的切换
- 切换return addr , kernel stack pointer , callee saved regs

**switch只需要保存14个寄存器，而非32个？**  因为swtch从C代码中调用，只需要保存callee saved register即可

**swtch中只是保存寄存器，而线程有很多其他状态如变量、堆栈等?**   这些数据都保存在内存中不会被干扰，因此只需要恢复寄存器即可。



##### 关于**锁p->lock的使用**

acquire(p->lock) release(p->lock)**流程**如下

- 从kernel thread -> scheduler
  - kernel thread yield 上的锁 由 切换到 scheduler的swtch之后 释放
- 从scheduler -> kernel thread
  - scheduler 上的锁 由 切换到 kernel thread的swtch之后 释放。

<img src="https://cstardust.github.io/2022/11/18/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F-xv6-thread/2022-11-22-15-38-54.png" alt="img" style="zoom:67%;" />

防止在我们这个线程切换还没完成的时候，另一个cpu核上的scheduler thread看到该进程为runnable并使用该线程。

###### XV6中，不允许进程在执行switch函数的过程中，持有任何其他的锁。所以，进程在调用switch函数的过程中，必须要持有p->lock（注，也就是进程对应的proc结构体中的锁），但是同时又不能持有任何其他的锁。

**原因**：进程P1调用了switch函数将CPU控制转给了调度器线程，调度器线程发现还有一个进程P2的内核线程正在等待被运行，所以调度器线程会切换到运行进程P2。假设P2也想使用磁盘，UART或者console，它会对P1持有的锁调用acquire，这是对于同一个锁的第二个acquire调用。当然这个锁现在已经被P1持有了，所以这里的acquire并不能获取锁。  同时进程2在自旋等待锁释放时会关闭中断，进而阻止了定时器中断并且阻止了进程P2将CPU出让回给进程P1



#### 在xv6中实现线程

所需要的信息都可以在用户空间保存

- 每个线程对应的栈
- 每个线程的状态
- 每个线程的寄存器信息(用于上下文切换)

```c
struct thread {
  char       stack[STACK_SIZE]; /* the thread's stack */
  int        state;             /* FREE, RUNNING, RUNNABLE */
  struct registers regs;
};
```



切换线程过程中，需要修改线程状态，并选出下一个运行的线程，调用汇编保存当前线程的callee-save registers，并恢复县一个线程的callee-save registers即可。完全可以类比于xv6内核中的进程切换。

