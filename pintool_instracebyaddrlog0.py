#!/usr/bin/env python
# -*- coding: utf-8 -*-

from subprocess import Popen, PIPE
from sys import argv
import string
import time

pinPath = "/home/kali/tools/pintools/pin-3.30-gcc-linux/pin"
pinInit = lambda tool,addroffest,elf: Popen([pinPath, '-t', tool,'-addroffest',addroffest,'--', elf], stdin = PIPE, stdout = PIPE)
pinWrite = lambda cont: pin.stdin.write(cont)
pinRead = lambda : pin.communicate()[0].decode()

flaglen = 0x2a
#初始化flag由!组成，ascii码是33
flag = bytearray(b'!' * flaglen)

if __name__ == "__main__":
    st = time.time()
    last = 0
    while True:
        pin = pinInit("inscountInaddr0/obj-intel64/instracebyaddrlog0.so", "0x1796" ,"./nor.nor")
        pinWrite(flag + b'\n')
        # print(pinRead())
        now = int(pinRead().split("Count ")[1],16)
        if(last != now):
            print(f"本位耗时:{time.time()-st},正确字符为：{chr(flag[last])}")
        last = now
        flag[last] += 1 

