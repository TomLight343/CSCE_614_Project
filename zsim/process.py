#!/usr/bin/python

from __future__ import division
import sys

filename = sys.argv[1]
fp = open(filename, "r")
wp = open("results.txt", "a")

cycleCount = 0
prelimCycleCount = 0
instructionCount = 0
missCount = 0
l3Count = 0
values = 0

for line in fp:
  values = line.split()
  if (values):
    if (values[0] == "cycles:"):
      prelimCycleCount += int(values[1])
    elif (values[0] == "cCycles:"):
      prelimCycleCount += int(values[1])
      cycleCount = max(cycleCount, prelimCycleCount)
    elif (values[0] == "instrs:"):
      instructionCount += int(values[1])
    elif (values[0] == "l3:"):
      l3Count = 1
    elif (values[0] == "mGETS:" or values[0] == "mGETXIM:" or values[0] == "mGETXSM:"):
      if (l3Count != 0):
        missCount += int(values[1])

wp.write(filename)
wp.write("\n")
wp.write(str(cycleCount))
wp.write("\n")
ipc = instructionCount/cycleCount
wp.write(str(ipc))
wp.write("\n")
mpki = 1000 * (missCount/instructionCount)
wp.write(str(mpki))
wp.write("\n\n")
fp.close()
wp.close()
