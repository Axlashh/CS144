-----

# 记录本人的cs144学习记录

lab应该是19年的版本，用的是kangyupl大佬的初始文件

网站看的是https://vixbob.github.io/cs144-web-page/

是fa20的版本，kangyupl大佬fa19的镜像网站貌似有点问题，有的文件我下不了

希望能学完

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
