#!/usr/bin/python
# -*- coding: utf-8 -*-

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("prgm", help="program that produced trace")
parser.add_argument("trace", help="trace to read")
parser.add_argument("-o", "--out", help="output file", default="tracesum.out")
args = parser.parse_args()
# check file existence
tmp = open(args.prgm)
tmp.close()
# analyze trace data
trace = open(args.trace)
# runmain stores the runtime address of main
# time stores the previous time
_, runmain, kernel, ptime = trace.readline().split(' ')
# for each function, a pair of
# (number of function calls, total time in function)
stat = {}
# add the first caller (the kernel)
stat[kernel] = [0, 0.0]
# whenever a function is called, add it to the stack
stack = []
# add main()
stack.append(runmain)
ptime = float(ptime)
trace.seek(0)
for data in trace:
    ex, callee, _, time = data.split(' ')
    time = float(time)
    if ex == 'e':
        if not callee in stat:
            stat[callee] = [0, 0.0]
        # function entry. Add the call count of callee and time of caller
        stat[callee][0] += 1
        # the caller is the top of the stack
        stat[stack[-1]][1] += time - ptime
        stack.append(callee)
    elif ex == 'x':
        # function exit
        stat[stack[-1]][1] += time - ptime
        stack.pop()
    ptime = time
trace.close()
# get the address of main in the elf binary
import subprocess
gobjdump = subprocess.Popen("gobjdump -d {0}".format(args.prgm).split(),
    stdout=subprocess.PIPE)
grep = subprocess.Popen("grep -m 1 _main".split(), stdin=gobjdump.stdout,
    stdout=subprocess.PIPE)
gobjdump.stdout.close()
elfmain = int(grep.communicate()[0].split(' ', 1)[0], 16)
gobjdump.wait()
# the slide offset
runmain = int(runmain, 16)
slide = runmain - elfmain
# total time
totaltime = 0.0
# begin output
out = open(args.out, "w+")
out.write("trace summary\n")
out.write("{0:32} {1:>16} {2:>16}\n".format("function", "call count", "ms"))
for fn, data in stat.iteritems():
    # symbolicate function address
    atos = subprocess.Popen("atos -d -s {0} -o {1} {2}".format(hex(slide),
     args.prgm, fn).split(' '), stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    symb, err = atos.communicate()
    sym = symb[0:symb.find('(')] + symb[symb.find(')')+1:-1]
    out.write("{0:46}{1:>16} {2:>16}\n".format(sym, data[0], data[1]))
    totaltime += data[1]
out.close()
print "wrote to {0}".format(args.out)
