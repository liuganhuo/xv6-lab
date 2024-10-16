## lec9



##### 中断interrupt与异常的区别：

中断通常与当前的执行的进程无关，转而去处理其他的请求

异常通常由当前的进程产生，包括fault(如page fault)与trap



##### 中断与系统调用的区别：

**asynchronous**：硬件发出Interrupt时，os的Interrupt handler与当前在cpu上运行的process没有任何关联。syscall发生在process的上下文中。

**并行性**：CPU与产生interrupt的设备是完全并行的

**program device**





#### 驱动

管理设备的代码称为驱动，所有的驱动都位于内核中。

##### 经典架构

- **top**：对应的用户进程或内核的其他部分调用的接口。主要是用来与内部进程产生一些交互
- **bottom**(interrupt handler)：当PLIC将中断送入CPU后，CPU会运行对应的interrupt handler进行处理。
- **buffer**：负责top与bottom之间的数据交流。

##### 为什么top不直接从interrupt handler中读取数据，而要经过buffer？

因为interrupt handler并没有运行在任何进程的上下文context中，所以进程中的page table并不知道要从哪里读取数据，因此需要一个buffer。





#### 中断流程

1. ##### 开启中断

三个目的：令设备可以发出中断，令PLIC可以接收并路由中断，令CPU可以接受中断

与中断相关的寄存器：

- SIE(Supervisor Interrupt Enable)：其中bit(E)代表其中外部中断。(S代表软件中断，T代表定时器中断)。负责特定中断。
- SSTATUS：打开、关闭中断，负责所有中断。
- SIP(Supervisor Interrupt Pending)：中断类型
- SCAUSE：中断原因
- STVEC：保存用户空间程序计数器

对应代码：

- **entry.S -> start.c : void start()**
  - 设置内核模式，设置SIE，启动中断
- **start.C -> main.c : int main(){}**，进入到内核初始化部分
  - consoleinit()：配置uart芯片，设置其中的寄存器信息，使uart可以产生中断
  - plicinit
- **plicinit,plicinithart** :
  - 使得PLIC可以接受相应中断，并路由到cpu。

- **cheduler -> inrt_on** 
  - 设置$SSTATUS，使得CPU可以接收中断

- 至此，中断被完全打开，cpu可以接受来自设备的中断。



##### 2. 中断时如何进入对应代码

1. 清除SIE，防止CPU核被其他中断打扰。CPU核完成当前中断后，再恢复$SIE
2. 保存PC到SPEC
3. 保存当前mode
4. 设置mode为superviosr mode
5. PC设置为STVEC，跳转到trap处理



##### 3. 实际执行(当没发生中断时)，如shell中打印$符号

**外部调用**：init.c -> sh.c -> getcmd -> fprintf($) -> (**进入系统调用**)  write(fd, &c, 1); -> sys_write -> filewrite -> devsw[CONSOLE].write -> 

**TOP层面**：consolewrite -> uartputc(c) ->

- 将字符写给buffer queue。通知uart处理char。

**BOTTOM层面**： uartstart()  (属于interrupt handler：uartintr() 的一部分)

- 将字符从buffer中取出，送入uart。之后系统调用返回，继续执行shell；同时uart发送数据

<img src="https://cstardust.github.io/2022/11/12/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F-xv6-Interrupt/2022-11-14-16-19-08.png" alt="img" style="zoom:33%;" />



##### 4. 实际执行(当发生中断时)，如shell从键盘读取输入并在屏幕上显示

PLIC产生中断，进入中断处理(见步骤2)。

- 从输入到终端显示：**usertrap -> devintr ->BOTTOM： uartintr ->uartgetc , consoleintr , uartstart**。
  - 在usertrap中判断是哪个设备的中断，进行相应处理。我们的例子中会进入uartintr()，也就是对应的interrupt handler
  -  uartgetc从uart中读取1个字符
  - consoleintr将从uart读取的字符填充到buffer queue中，并唤醒consoleread。
  - uartstart将buffer中的字符取出，送入uart中，进而在终端中显示
- 另一部分，进程也要读取到对应的输入： (sh.c)gets -> read -> fileread -> devsw[f->major].read -> consoleread(int user_dst, uint64 dst, int n) ->阻塞的读完指定bytes
  - 其中consoleread就对应了TOP部分，当buffer中存入了数据后，继续读取



<img src="https://cstardust.github.io/2022/11/12/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F-xv6-Interrupt/2022-11-14-16-18-22.png" alt="img" style="zoom:33%;" />

#### interrupt并发

**设备与CPU是并行运行的**：如CPU将字符送入UART的reg之后就会返回Shell，而与此同时，UART正在并行的将字符传送给Console。此时Shell可能正在并行的调用write，向buffer queue中加入字符。

**中断会停止当前运行的程序**：中断会导致跳转到uservec trampoline，进行和syscall trap时一样的对用户上下文的保存。

**驱动的top和bottom部分是并行运行的**：例如，Shell会在传输完提示符“$”之后再调用write系统调用传输空格字符，代码会走到UART驱动的top部分（注，uartputc函数），将空格写入到buffer中；但是同时在另一个CPU核，可能会收到来自于UART的中断，进而执行UART驱动的bottom部分，查看相同的buffer。

<img src="https://cstardust.github.io/2022/11/12/%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F-xv6-Interrupt/2022-11-14-17-01-58.png" alt="img" style="zoom:50%;" />



 









