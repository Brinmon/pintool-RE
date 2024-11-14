
# PinCracker pintools侧信道攻击脚本
## 环境要求
pintool不同版本的下载地址:https://www.intel.com/content/www/us/en/developer/articles/tool/pin-a-binary-instrumentation-tool-downloads.html
目前使用版本:https://software.intel.com/sites/landingpage/pintool/downloads/pin-external-3.31-98869-gfa6f126a8-gcc-linux.tar.gz
将压缩包下载下来之后直接解压到目标目录再添加环境变量就可以了!
```bash
# 添加Pin工具到PATH
export PIN_ROOT=~/tools/pintool
export PATH=$PIN_ROOT:$PATH
```
## inscountInaddr0 : 
```d
┌──(kali㉿kali)-[~/GithubProject/pintool-RE/inscountInaddr0]
└─$ ls
instracebyaddrlog0.cpp  makefile  makefile.rules
```

makefile.rules:这个文件目前就instracebyaddrlog0有用,用来给编译出来的so文件命名!
```d
...
# 这定义了运行同名工具的测试。这只是为了方便避免两次定义测试名称（一次在TOOL_ROOTS中，另一次在TEST_ROOTS中）。
# 这里定义的测试不应在TOOL_ROOTS和TEST_ROOTS中定义。
TEST_TOOL_ROOTS := instracebyaddrlog0
...
```

makefile:这文件需要配置一下pintool的路径,我添加了`$(PIN_ROOT)`所以如果设置了环境变量就不需要修改!
```d
...
# If the tool is built out of the kit, PIN_ROOT must be specified in the make invocation and point to the kit root.
PIN_ROOT :=$(if $(PIN_ROOT),$(PIN_ROOT),/home/kali/tools/pintools/pin-3.30-gcc-linux)
ifdef PIN_ROOT
CONFIG_ROOT := $(PIN_ROOT)/source/tools/Config
else
CONFIG_ROOT := ../Config
endif
include $(CONFIG_ROOT)/makefile.config
include makefile.rules
include $(TOOLS_ROOT)/Config/makefile.default.rules
...

```

instracebyaddrlog0.cpp:就是我编写的动态劫持库了
```c++
KNOB< ADDRINT > KnobTargetAddressOffest(KNOB_MODE_WRITEONCE, "pintool", "addroffest", "0", "指定目标插桩位置的偏移");

// ImageLoad回调函数
VOID ImageLoad(IMG img, VOID* v)  //获取elf的基地址解决无法hook开启了pie的程序
{}
// 每次遇到新指令时，Pin调用这个函数
VOID Instruction(INS ins, VOID* v)//对每个指令进行插桩
{}


int main(int argc, char* argv[])
{
    // 初始化Pin
    if (PIN_Init(argc, argv)) return Usage();

    // 注册ImageLoad回调函数
    IMG_AddInstrumentFunction(ImageLoad, NULL);

    // 注册Instruction函数以便插装指令
    INS_AddInstrumentFunction(Instruction, 0);

    // 注册Fini函数以便在应用程序退出时调用
    PIN_AddFiniFunction(Fini, 0);

    // 启动程序，永不返回
    PIN_StartProgram();

    return 0;
}

```

再环境装配完成之后就可以开始编译了!
```d
#编译32位
└─$  make obj-ia32/instracebyaddrlog0.so TARGET=ia32
或者 make all TARGET=ia32
#编译64位
└─$ make obj-intel64/instracebyaddrlog0.so TARGET=intel64 
或者 make all TARGET=intel64 
```


// 注册Instruction函数以便插装指令
INS_AddInstrumentFunction(Instruction, 0);
通过对每条汇编指令插入回调函数来实现程序插桩，INS_InsertCall
![alt text](photo/imageinscountInaddr0.png)

