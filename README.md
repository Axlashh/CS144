-----

# 记录本人的cs144学习记录

lab应该是19年的版本，用的是kangyupl大佬的初始文件

网站看的是https://vixbob.github.io/cs144-web-page/

是fa20的版本，kangyupl大佬fa19的镜像网站貌似有点问题，有的文件我下不了

希望能学完

### 2024/2/21

下面做个debug总结

- active_close

tcp header too short

排查之后发现是tick之后sender认为超时了，于是执行超时重传，但此时没有seg需要重传，于是传了一段奇怪的内存段过去

在超时重传的逻辑增加了重传列表不为空才重传，active_close就过了

- passive_close

The TCP an extra segment when it should not have

是在test 2中，进入close_wait后tick了4倍超时时间，再close和tick 1ms后检查是否发送了fin时报的错，推测是重传又出问题了

果然是重传的问题，之前写sender的时候偷懒，没写计时器的开关，导致没有发送数据时计时器也会空转，加了个开关，passive_close也过了

- t_ack_rst

The TCP should have produced a segment that existed, but it did not

这个bug貌似是说当收到了尚未发送的序列号的ACK时，要回一个ack回去???

好像和之前学的不一样，不应该是忽略吗

后面几个测试的意思是只要收到了带有内容的seg，就要回个ack

是文档里写了但我没看见吗，好像之前没有说有这个逻辑，加了之后过了部分

然后到了回复rst的bug，是不能回复窗口以外的rst，并且listen状态时不管rst，改了之后过了

test3是各种接收，第一个测试是处于listen状态时，如果只收到ack，则进rst

但我的sender以为进正常传输阶段了，直接崩掉

加了个listen状态的判断后，又发现sender发了两个rst

排查出来是我在unclean_shutdown()里加了一个send_empty_segment，删掉后过了

test4要求在syn_sent状态中如果收到带信息的seg，就不管它

如果在syn_sent状态收到只含rst的seg，不管它；如果收到正确的act+rst，进入reset状态但是不发送rst

我服了，细节真的多

ok，现在是凌晨一点，我彻底晕了，单独运行文件跟直接make出来的结果不一样，一边是要求syn_sent状态下，收到不正确的ack就直接rst，并且发送的rst的seqno和接收到的ackno一样，和在listen状态下，收到ack就直接rst；另一边是这两个情况下都不管。我已经麻了，网上关于这些方面的资料还少，代码已经成屎山了，很绝望。mmd，先把make过了再说

好好好，找到原因了，make里面用的是fsm_ack_rst_relaxed这个文件，要求的是宽松判断，不需要回rst，但我一直对着fsm_ack_rst这个文件来改，是比较严格的判定。这下给我整不会了，怪不得在大佬们的代码里也没翻到这个逻辑，原来是压根就没有，还是按make的要求来吧，自己知道什么时候回rst就好。

改成了listen状态下忽略ack和syn_sent状态下忽略错误的ack，t_ack_rst过了

- t_connect

这次我学聪明了，先确定他用的文件是relaxed版本

如果在syn_sent状态下，先收到单独的syn，是要回一个ack进入syn_recv状态的，但我没回

加了判断进入syn_recv的逻辑后解决这个bug

又接着说我在syn_recv状态下收到ack后回了个ack，查半天发现是我把双方的isn搞混了，应该是我们的isn和我们的seqno和对方的ackno是一起的，结果我用对方的isn来unwrap对方的ackno，这个bug才查出来真是个奇迹，改完后t_connect过了

- t_listen

在listen状态下tick后我主动发送了syn，改成listen状态下的tick不发送syn

接着是listen状态下接收到syn后多发了seg，发现是我检测到接收到的seg有syn就发送一个空seg，调用_sender的fill_window()后sender检测到没发过syn，就又发了一个syn，改掉后好了

接着是syn_recv状态下关于接收到的ack的问题，如果seqno不对，则不连接并回复一个ack，只有seqno和ackno都对的情况下才进入established状态。

tm改死我了，真的是写了一坨屎山，不敢乱动，动一下就出问题，最后是在reciver里加了length_in_sequence_space() == 0并且seqno == 窗口右端（合法的ack）的情况下就认为这个seq合法，并且只要接收的seg不对就要回个ack，才过的

这里写完之后通过率大幅上升，终于看到一点希望了TAT，虽然还是只有百分之五十几（

- t_retx

发现是反复超时那里有问题，是我之前把unclean_shutdown学大佬的封装成了一个函数，结果反复超时的那里没调用这个函数，用的是之前错误的写法，改掉之后就过了

- 真实通信

然后貌似有文件的test就都通过了，我暴风哭泣，check但是貌似还有大问题，t_i开头的测试统统failed，t_u的128K_8K也统统timeout，应该就是两个问题，报的错都一样，但nm怎么找啊

我试着把connection和sender都替换成大佬的，发现超时问题解决了，但是还是统统failed，全部替换完还是都failed...

根据huangrt01大佬的指示

* “c” means your code is the client (peer that sends the first syn)
* “s” means your code is the server.  
* “u” means it is testing TCP-over-UDP
* “i” is testing TCP-over-IP(TCP/IP). 
* “n” means it is trying to interoperate with Linux’s TCP implementation
* “S” means your code is sending data
* “R” means your code is receiving data
* “D” means data is being sent in bothdirections
* lowercase “l” means there is packet loss on the receiving (incoming segment) direction
* uppercase “L” means there is packet loss on the sending (outgoing segment) direction.

结果重启之后t_i没问题了，出现大约十几个timeout，均是在128K_8K有丢包情况下出现的问题

典型报错如下

```
DEBUG: Connecting to 169.254.144.1:2781... done.
DEBUG: Inbound stream from 169.254.144.1:2781 finished cleanly.
DEBUG: Outbound stream to 169.254.144.1:2781 finished (52272 bytes still in flight).
```

根据Kiprey大佬的指示，使用wireshark进行了抓包再分析

分析出来貌似是传输发生丢包时，tcp会先猛发送，直至把窗口填满，窗口大小往往都很大，就导致每次塞完窗口后，都会有三四个丢包。但是接收方当窗口满时，有时会把自己的窗口翻倍，而发送方会优先往后发送，而不是重传。这导致当sender发送完时，往往还会有三四个丢包，但sender只会重传一个，然后就结束链接了。所以我猜测是重传方面出问题

OK!!!OK!!!我nm哭了，对着大佬的代码一行一行看，改出来了！！！bug原因是我收到ack后，会把计时器停止，直到下一次发送新seg时再开启。但是到最后只有几个等待重传的seg时，sender收到一个ack，就停止计时了，导致后面的seg都没法重传。把逻辑改为函数结束时判断还有没有未接收的seg，如果有就启动计时器，然后就全部通过啦哈哈哈！！！（虽然我的sender现在已经快是一比一复刻huangrt01大佬的了）

### 2024/2/20

tm这个lab4不如杀了我

全都是错的，还一堆莫名其妙的bug

tcp header too short到底是啥？我硬是找不出来，代码里也看不到，又nm有问题，真的折磨

### 2024/2/19

今天把第四章 congestion control看完了，写点笔记

接下来应该可以做lab4了

- AIMD (Additive Increase Multiplicative Decrease)

	如果传输正常，接受到ack，则线性增长 (w = w + 1/w)

	如果有丢包，则乘法性的减小 (w = w/2)

	对于single flow来说，若传输瓶颈的速率为c，则当router的buffer size = RTT * c时，利用率可维持在100%

	传输速率 $r = \frac{w}{RTT}$ ，是个常量

	对于multi flow来说，由于buffer可视为一直是满的，则RTT不随窗口大小w的改变而改变

	丢包率 $p = \frac{8}{3 W_{max}^2}$

	传输速率 $r = \frac{A}{\frac{W_{max}}{2} RTT} = \sqrt{\frac{3}{2}}\frac{1}{RTT \sqrt{p}}$

	好处：Chiu Jain Plot

- TCP Tahoe Reno NewReno (放一起得了）

	- slow start

		发送窗口大小以Maximum Segment Size(MSS)开始，进行指数式增长(*2)直到丢包或是3个重复ack或是超过ssthresh

	- Congestion Avoidence

 		线性增长

   	- RTT Estimate
 
		r为估计值，g为EWMA gain，m为最近一次测量值

		e = m - r

		r = r + g * e

		v = v + g * (|e| - v)

		Timeout = r + $\beta$ v ( $\beta$ = 4)

	- Self Clocking

 		指的是tcp只会在收到ack后才发送新seg，发送速率与确认速率同步

   	- Fast Recovery and Retransmit
 
		当处于Congestion Avoidence阶段时，若收到大于等于三个重复ack时，则进入快速重传，立即发送未收到的seg，ssthresh设为w/2，若中途还收到重复ack，则窗口大小w加一，直到收到新的ack

		收到新ack后，不是将w设为MSS，而是设为ssthresh，然后继续Congestion Avoidence

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
