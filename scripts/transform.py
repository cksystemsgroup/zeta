import sys;

data = open("ops.txt")

ops = [{"type":line.split(" ")[0], "value":int(line.split(" ")[1]),
"start":int(line.split(" ")[2]), "lin":int(line.split(" ")[3]),
"end":int(line.split(" ")[4])} for line in data]

nextValue = 2

while 1:
  lowest = ops[0]
  time = "start"
  value = 2180652018688081
  done = True

  for op in ops:
    if op["start"] > 1000:
      done = False
      if op["start"] < value:
        lowest = op
        time = "start"
        value = op["start"]
      elif op["start"] == value:
    if op["end"] > 1000:
      done = False
      if op["end"] < value:
        lowest = op
        time = "end"
        value = op["end"]
      elif op["end"] == value:

  if not done: 
    lowest[time] = nextValue
    nextValue = nextValue + 2

  if done:
    break
 
values = []

for op in ops:

  found = False

  for i in range(len(values)):
    if values[i] == op["value"] :
      found = True
      op["value"] = i + 1

  if not found:
    values.append(op["value"])
    op["value"] = len(values) 

for op in ops:
  break
  if op["type"] == '+':
    print "{value} {lin}|".format(value= str(op["value"]).rjust(2),
        lin = ''.join([' ']*op["start"]) + '|' + ''.join(['_']*(op["end"] -
          op["start"] - 1)))

for op in ops:
  break
  if op["type"] == '-':
    print "{value} {lin}|".format(value= str(op["value"]).rjust(2),
        lin = ''.join([' ']*op["start"]) + '|' + ''.join(['_']*(op["end"] -
          op["start"] - 1)))

        
for op in ops:
  print "{optype} {value} {start} {lin} {end}".format(
      optype = op["type"],
      value = op["value"],
      start = op["start"],
      lin = 0,
      end = op["end"])


