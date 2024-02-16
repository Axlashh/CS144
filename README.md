-----

# 记录本人的cs144学习记录

lab应该是19年的版本，用的是kangyupl大佬的初始文件

网站看的是https://vixbob.github.io/cs144-web-page/

是fa20的版本，kangyupl大佬fa19的镜像网站貌似有点问题，有的文件我下不了

希望能学完

### 2024/2/15

今天直接麻中麻，没想到后面还难一点，并且好像书上没有相关的内容，随便写了点笔记附在下面了

### 2024/2/14

今天看了3-1到3-6，看得我困麻了，写点笔记

- Circuit Switching

  	一对一用circuit连接的通信

	1. 每个通信间都是private, guaranteed, isolated

	2. 每次通信都要维护状态

  	问题：

   	1. inefficient，效率低下
 
   	2. 不同通信间速率要求变化大，不好满足
 
   	3. 要维护通信状态

- Packet Switching

	端与端间以packet的形式通信

- Delay

	- Propagation Delay

   		来源于信号在link中传输所带来的延迟

 		$t_l = \frac{l}{c}$

  		一般来说c取 $2 * 10^8 m/s$

   	- Packetization Delay
   	
   	  	一组数据进入link所带来的延迟

		$t_p = \frac{p}{r}$

      	- Queue Delay
   	 
   	  	一堆数据在router中排队等待发送带来的延迟，等下详细讨论

- Playback Buffer

  	视频网站这种Real-time application，为了避免Queue Delay波动大带来的不稳定，会缓存

- Queue Model

	- Q(t)

  		假设A(t)为t时刻之前到达queue的bit，D(t)为离开的

  		则Q(t) = A(t) - D(t)，即为t时刻队列中所含的bit数

  		d(t)为垂直的差值，代表同一个bit花了多久离开

  		$\overline{Q}(t) = \frac{\int_0^T{Q(t)}}{T}$  其中T为周期

    	- 不同发送策略的时间差距

		一次发一整个: $t = \sum{\frac{M}{r_i} + \frac{l_i}{c}}$ M代表数据量

		分很多packet: $t = \sum{\frac{p}{r_i} + \frac{l_i}{c}} + (\frac{M}{p} - 1) * \frac{p}{r_{min}}$ p代表每个packet的数据量，$r_{min}$代表传输最慢的那根线

	- Statistical Multiplexing

   		不同数据源传输的峰值时间不太一样，根据实际需求而不是峰值相加来分配输出速率

   		~~我也没太搞懂~~

- Queue Properties

	- Burstiness increases delay

 	- Determinism minimizes delay

 	- Little's result

		我一开始不知道Little是个人名，也不知道result有法则的意思，疑惑了半天（

		$L = \lambda W$

		L为队列中的平均信息量，$\lambda$为信息到达的平均速度，W为信息离开的平均速度

	- M/M/1 model

 		视频里还没讲太清楚，我找了点别的资料

 		假设进入队列的信息量符合泊松分布($\lambda$)，信息离开的时间符合指数分布($\mu$)

   		则d = $\frac{1}{\mu - \lambda}$
 
   		设 $\rho = \frac{\lambda}{\mu}$ ，代表服务强度

   		$L = \lambda d = \frac{\rho}{1 - \rho}$

- Packet Switch

 	- 基本要做的事

		1. Lookup addresses in a forwarding table

		2. Switching the correct egress port

 	- 不同的缓冲策略
 
 		1. Output Queue

  		2. Input Queue
 
  		3. Virtual Output Queue

	- 不同的入队策略

 		1. FIFO queue

		2. Strict Priorities

			分为两个队列，一个高优先级一个低优先级，先出高优先级再出低优先级

		3. Weighted Fair Queueing

			......

有蛮多排队论的知识，真就是学数学了，没想到概率论还能在这里用上

明天争取把第三章看完，再看点第四章，回家就做lab4了

	

### 2024/2/13

哈哈，解决lab3!

其实bug都是在一些小细节上，但没写好就全gg

一个是窗口中的内容是已发送但还未确定的，所以考虑发送内容时要把next_seqno_absolute - acknowledged_seqno当成占据了窗口的一部分，不然就会猛往后面发。

还有发送完fin后就不能发送新seg了，要发也只能是重传，不然就会发137个fin标志出去（属实nt

retransmission_time初始值不能设0（sb

初上手时实在没头绪，看了一点kangyupl大佬的代码，不过后面基本都是自己改出来的

下面争取全都自己写！

终于能肆无忌惮地往后看课了，虽然感觉课中内容和lab没啥联系了已经，反正tcp都学完了

### 2024/2/12

服了，打个sb美赛搞得心力憔悴，队友纯nt，透支了又导致后面没动力学

然后就回来过年了，这初三晚上总算抽了点时间出来写lab3

文档我来来回回看了五六遍才开始写，真是都靠自己

说是arq协议，看了几遍才发现是说只要收到了一个ack就代表之前的序列全都已收到，跟课上讲的那三种又不同

还研究了几遍握手过程，现在才算是彻底明白ack和fin都算是seq中的一个字节，也要占用窗口大小

现在是只写了个大概，一个test都过不了，一堆bug等着修，服了

### 2024/2/1

今天光看课去了，看得我想睡觉

但是看了一堆，把tcp的go back n 和 selective repeat都看完了，后面貌似是packet switching之类的东西

搞不太懂cs144课程网站里又是怎么安排的，tcp的ppt就几页，然后直接进packet switching，但是项目又全是关于tcp的

应该能写tcp sender了，今天看了一堆，小小写点笔记

- TCP
  	1. TCP Header
  
  	包含一堆field，主要就是source port, destination port, sequence, ackno, checksum和一堆flag

  	2. TCP FSM
 
  	那几个状态，有时间默一遍

  	3. Flow Control
 
  		- Stop and Wait
 
  		太傻了，发出去后等收到ack回应了再发下一个，利用one byte counter来分辨是新的还是重传发的

  		- Go Back N
 
  		发送窗口大小为n，接受窗口大小为1

  		接受窗口会发送连续上升序列的最后一个序号，发送方会发送ackno的后n个segment

		- Selective Repeat

    		发送窗口只重传没有收到确认的segment
  	  
- UDP

UDP Header只包含source destination checksum length，相当于给IP做了个wrapper

- ICMP

传输层的一部分，相当于用来给IP传输纠错的

包含IP DATA的前8个字节和IP HEADER，再加上TYPE和CODE用来标识错误信息

可以用来ping和查找传输路径上的router

- 纠错
	1. checksum

  	IP TCP等 把DATA里的字节相加作为checksum

	2. Cycle Roundency Check

 	Ethernet等 选取一个generator polynomial，将数据除以G的余数作为CRC段加在后面，接受的时候检查数据除以G的余数是否等于0

  	3. Message Authentication Code

      	TLS等 选一个serect s来生成一段mac，貌似主要是用于确保数据安全的，纠错能力不强

大概就是这些，希望过几天看的时候都能回忆起来

接下来继续写lab

### 2024/1/31

今天把lab2干完了

发现课有一大堆等着看

tcp的基本原理他一节课就基本介绍完了，有点出乎意料，但后面貌似还有好多细节

今天这个lab的细节也调死我了，一堆边界判断，还发现以前写的东西不太符合要求，搁那猛改

一个难点是结合两个标志位一起判断滑动窗口的边界，这里偷了康宇大佬的_base

但感觉还是有点问题，现在的逻辑是只要segment中含有标志位就接受，但如果其中含有FIN位，而absolute seqno超出了滑动窗口的范围，那它这段segment所含的信息就没了

想回来貌似又没问题，反正有ackno，信息能再要一遍，没收到FIN位就麻烦了

nice，美美睡觉

### 2024/1/30

今天把lab1干完了

总体来说前两个lab都挺简单的，不过lab1一开始是用map写的，后来想优化一下，用vector写，结果因为没考虑字节流里可能有空字符，让我找了老久bug

但是油管上的网课感觉不够配套，英语不好就看得贼慢，前面还都是介绍原则跟大小端之类的，跟lecture也不一样，慢慢看吧

### 2024/1/29

做各种初始准备 

git的操作都忘得差不多了，整老半天

接下来看看视频


29号晚上直接写到30号一点了

结束lab0，小小热身，捡回自己的编程

getURL主要是得读文档，搞清函数都是干嘛的

bytestream就是小小的实现功能，不过花了好长时间debug size_t的问题

size_t是无符号整型，减到负数会溢出，mmd

而且一开始折腾好久配环境，make的时候显示throw out_of_range什么的有问题

最后只要在buffer.hh里加个#include <stdexcept>就好了，真是服

今天还不错，开睡！

-----

For build prereqs, see [the CS144 VM setup instructions](https://web.stanford.edu/class/cs144/vm_howto).

## Sponge quickstart

To set up your build directory:

	$ mkdir -p <path/to/sponge>/build
	$ cd <path/to/sponge>/build
	$ cmake ..

**Note:** all further commands listed below should be run from the `build` dir.

To build:

    $ make

You can use the `-j` switch to build in parallel, e.g.,

    $ make -j$(nproc)

To test (after building; make sure you've got the [build prereqs](https://web.stanford.edu/class/cs144/vm_howto) installed!)

    $ make check

The first time you run `make check`, it will run `sudo` to configure two
[TUN](https://www.kernel.org/doc/Documentation/networking/tuntap.txt) devices for use during
testing.

### build options

You can specify a different compiler when you run cmake:

    $ CC=clang CXX=clang++ cmake ..

You can also specify `CLANG_TIDY=` or `CLANG_FORMAT=` (see "other useful targets", below).

Sponge's build system supports several different build targets. By default, cmake chooses the `Release`
target, which enables the usual optimizations. The `Debug` target enables debugging and reduces the
level of optimization. To choose the `Debug` target:

    $ cmake .. -DCMAKE_BUILD_TYPE=Debug

The following targets are supported:

- `Release` - optimizations
- `Debug` - debug symbols and `-Og`
- `RelASan` - release build with [ASan](https://en.wikipedia.org/wiki/AddressSanitizer) and
  [UBSan](https://developers.redhat.com/blog/2014/10/16/gcc-undefined-behavior-sanitizer-ubsan/)
- `RelTSan` - release build with
  [ThreadSan](https://developer.mozilla.org/en-US/docs/Mozilla/Projects/Thread_Sanitizer)
- `DebugASan` - debug build with ASan and UBSan
- `DebugTSan` - debug build with ThreadSan

Of course, you can combine all of the above, e.g.,

    $ CLANG_TIDY=clang-tidy-6.0 CXX=clang++-6.0 .. -DCMAKE_BUILD_TYPE=Debug

**Note:** if you want to change `CC`, `CXX`, `CLANG_TIDY`, or `CLANG_FORMAT`, you need to remove
`build/CMakeCache.txt` and re-run cmake. (This isn't necessary for `CMAKE_BUILD_TYPE`.)

### other useful targets

To generate documentation (you'll need `doxygen`; output will be in `build/doc/`):

    $ make doc

To lint (you'll need `clang-tidy`):

    $ make -j$(nproc) tidy

To run cppcheck (you'll need `cppcheck`):

    $ make cppcheck

To format (you'll need `clang-format`):

    $ make format

To see all available targets,

    $ make help
