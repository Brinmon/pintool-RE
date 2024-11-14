#!/usr/bin/env python
# -*- coding: utf-8 -*-

from subprocess import Popen, PIPE
from sys import argv
import string
import time
import os

# 通过环境变量获取 pin 工具所在的目录路径，并拼接 'pin'
pinPath = os.getenv('PIN_ROOT', '/home/kali/tools/pintool')  # 获取目录路径
if pinPath:
    pinPath = os.path.join(pinPath, 'pin')  # 拼接 pin 文件名
else:
    pinPath = '/home/kali/tools/pintool/pin'  # 默认路径

pinInit = lambda tool, addroffest, elf: Popen([pinPath, '-t', tool, '-addroffest', addroffest, '--', elf], stdin=PIPE, stdout=PIPE)
pinWrite = lambda cont: pin.stdin.write(cont)
pinRead = lambda: pin.communicate()[0].decode()

flaglen = 0x2a
# 初始化flag由!组成，ascii码是33
flag = bytearray(b'!' * flaglen)
flagchartable = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-{}"

if __name__ == "__main__":
    st = time.time()
    idx = 0
    now = 0
    while True:
        for char in flagchartable:  # 尝试每个字符
            pin = pinInit("inscountInaddr0/obj-intel64/instracebyaddrlog0.so", "0x1796", "./nor.nor")
            flag[idx] = ord(char)
            pinWrite(flag + b'\n')  # 发送更新后的flag
            now = int(pinRead().split("Count ")[1], 16)
            print(f"当前爆破字符为：{char}，当前flag为：{flag.decode()}")
            if idx != now:  # 如果输出发生变化，说明字符正确
                idx += 1  # 转到下一个字符
                break  # 进入下一轮循环
        if idx != now:
            print(f"本位耗时:{time.time()-st},正确字符为：{chr(flag[idx])},剩余字段爆破失败!")
            break
        print(f"正确字符为：{char}，当前flag为：{flag.decode()}")

        if(idx == flaglen):
            print(f"本位耗时:{time.time()-st},正确字符为：{chr(flag[idx])}")
        

