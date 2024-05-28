/*
 * 版权所有 (C) 2004-2021 英特尔公司。
 * SPDX许可证标识符：MIT
 */

#include <iostream>
#include <fstream>
#include "pin.H"
using std::cerr;
using std::endl;
using std::ios;
using std::ofstream;
using std::string;

//要插桩的地址
ADDRINT targetAddress;
// 全局变量，用于存储程序和libc的基址
ADDRINT main_base = 0;
ADDRINT libc_base = 0;

// 这里保存指令的运行计数
// 将其设为静态以帮助编译器优化docount
static UINT64 icount = 0;

// 调试输出开关
bool DEBUG_ENABLED = false;

// 输出调试信息
#define DEBUG_LOG(message) \
    if (DEBUG_ENABLED) { \
        std::cout << "[DEBUG] " << message << std::endl; \
    }

KNOB< ADDRINT > KnobTargetAddressOffest(KNOB_MODE_WRITEONCE, "pintool", "addroffest", "0", "指定目标插桩位置的偏移");

// 这个函数在每个代码块之前调用
inline VOID docount(UINT32 c) { icount += c; }

VOID ImageLoad(IMG img, VOID* v)
{
    if (targetAddress != 0) return;  // 如果目标地址尚未设置，则跳过

    // 获取程序基址
    if (IMG_IsMainExecutable(img)) {
        main_base = IMG_LowAddress(img);
        DEBUG_LOG("Main executable base address: " << std::hex << main_base)
        targetAddress = main_base + KnobTargetAddressOffest.Value();
        DEBUG_LOG("目标插桩地址： 0x" << std::hex <<targetAddress)
        return;
    }
    // 获取libc基址
    else if (IMG_Name(img).find("libc") != std::string::npos) {
        libc_base = IMG_LowAddress(img);
        DEBUG_LOG("libc base address: " << std::hex << libc_base)
        return;
    }
}


// 提取最高的非零字节以及所有前导零
uint64_t getTopNonZeroByte(uint64_t value) {
    if (value == 0) {
        return 0;
    }

    // 找到最高的非零字节位置
    int shiftAmount = 0;
    for (int i = 56; i >= 0; i -= 8) {
        if ((value >> i) & 0xFF) {
            shiftAmount = i;
            break;
        }
    }

    // 提取最高的非零字节及其前导零
    return (value >> shiftAmount) & 0xFF;
}

// 每次遇到一个新的基本块时，Pin调用这个函数
// 它插入一个对docount的调用
VOID Trace(TRACE trace, VOID* v)
{   
    //什么是基本块（Basic Block）
    //1.基本块是程序中一个直线型代码序列
    //2.单一入口：执行进入基本块的第一条指令后，所有后续指令都将被依次执行，直到基本块的最后一条指令。
    //3.单一出口：基本块中的最后一条指令是跳转指令或函数返回指令，或者是程序的结束指令。在此之前，没有其他的跳转或分支指令。
    // 遍历trace中的每个基本块
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // 获取基本块的起始地址c
        ADDRINT bblAddr = BBL_Address(bbl);
        if(getTopNonZeroByte(bblAddr) == 0x7f){
            return;
        }
        if (bblAddr == targetAddress) {
            //用来验证插桩的位置的汇编指令是否正确
            DEBUG_LOG("Instruction Address: 0x" << std::hex << INS_Address(BBL_InsHead(bbl)) << ", Assembly: " << INS_Disassemble(BBL_InsHead(bbl)))
            // 在每个基本块之前插入一个对docount的调用，传递指令数
            BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)docount, IARG_UINT32, 1 , IARG_END);
            return;
        }
    }
}


// 当应用程序退出时调用这个函数
VOID Fini(INT32 code, VOID* v)
{
    // 写入文件，因为应用程序可能已经关闭了cout和cerr
    std::cout << "Count " << std::hex << std::showbase << icount << endl;
}

/* ===================================================================== */
/* 打印帮助信息                                                          */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "这个工具统计执行的动态指令数" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* 主函数                                                                */
/* ===================================================================== */

int main(int argc, char* argv[])
{
    // 初始化Pin
    if (PIN_Init(argc, argv)) return Usage();

    // 注册ImageLoad回调函数
    IMG_AddInstrumentFunction(ImageLoad, NULL);

    // 注册Trace函数以便插装指令
    TRACE_AddInstrumentFunction(Trace, 0);

    // 注册Fini函数以便在应用程序退出时调用
    PIN_AddFiniFunction(Fini, 0);

    // 启动程序，永不返回
    PIN_StartProgram();

    return 0;
}
