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
ADDRINT targetAddress = 0;
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

// ImageLoad回调函数
VOID ImageLoad(IMG img, VOID* v)
{
    if (targetAddress != 0) return;  // 如果目标地址尚未设置，则跳过

    // 获取程序基址
    if (IMG_IsMainExecutable(img)) {
        main_base = IMG_LowAddress(img);
        DEBUG_LOG("Main executable base address: " << std::hex << main_base)
        targetAddress = main_base + KnobTargetAddressOffest.Value();
        DEBUG_LOG("目标插桩地址： 0x" << std::hex <<targetAddress)
    }
    // 获取libc基址
    else if (IMG_Name(img).find("libc") != std::string::npos) {
        libc_base = IMG_LowAddress(img);
        DEBUG_LOG("libc base address: " << std::hex << libc_base)
    }
}

// 这个函数在每次指令执行之前调用
inline VOID docount() { icount++; }

// 每次遇到新指令时，Pin调用这个函数
VOID Instruction(INS ins, VOID* v)
{
    ADDRINT addr = INS_Address(ins);  
    if (addr == targetAddress) {
        //用来验证插桩的位置的汇编指令是否正确
        DEBUG_LOG("Instruction Address: 0x" << std::hex << INS_Address(ins)<< ", Assembly: " << INS_Disassemble(ins))
        // 在每条指令之前插入对docount的调用，不传递任何参数
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);
    }
    return;
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
/* argc和argv是完整的命令行：pin -t <工具名> -- ...                     */
/* ===================================================================== */

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
