import os;
import sys;
import math;

def calcDistribution(filename, outputFilename):
  content = open(filename, 'r').readlines()

  if len(content) == 0:
    print "Invalid file {filename}".format(filename = filename)
    return

  numOps = 10000
  first_array = [0] * 100
  last_array = [999999999999] * 100
  index = 0
  total = 0

  for line in content:
    if line.startswith('-'):
      index = index + 1
      parts = line.split(' ')
      value = int(parts[1])
      thread = (value - 1) / numOps
      if thread < 0 or thread >= 100:
        print "Invalid thread {id} in file {filename}".format(id = thread, filename = filename)
        return
      else :
        if first_array[thread] == 0:
          first_array[thread] = index
        last_array[thread] = index


  startIndex = max(first_array)
  endIndex = min(last_array)

  histogram = [0] * 301
  index = 0
  total = 0
  positions = [0] * 100001

  for line in content:
    if line.startswith('-'):
      index = index + 1
      if index >= startIndex and index <= endIndex:
        parts = line.split(' ')
        value = int(parts[1]) - 1
        thread = value / numOps
        element = value % numOps + 1
        if thread == 1:

          element = value % numOps + 1
          positions[element] = index

          if (positions[element - 1] != 0) :
            hist_index = positions[element] - positions[element - 1] + 100
            if hist_index < 0:
              hist_index = 0
            elif hist_index > 300:
              hist_index = 300

            histogram[hist_index] = histogram[hist_index] + 1

          elif (positions[element + 1] != 0) :
            hist_index = positions[element + 1] - positions[element] + 100
            if hist_index < 0:
              hist_index = 0
            elif hist_index > 300:
              hist_index = 300

            histogram[hist_index] = histogram[hist_index] + 1

  output = open(outputFilename, 'w')
  for i in range(0, 301):
    output.write ("{bin} {value}\n".format(bin = (i - 100), value = histogram[i]))

  output.close()

import os;
import multiprocessing;
import subprocess;
import ah_config;
import sys;

def startCalculation(inputData):

  item = inputData["filename"]
  filename = "{directory}{filename}".format(filename = os.path.basename(item), directory = inputData["directory"])

  print ("{item}; {resultfile}".format(item = item, resultfile = filename))
  calcDistribution(item, filename)

def calcOpFairness(in_dir, out_dir):
 
  if not in_dir.endswith('/') :
    in_dir = in_dir + '/'

  if not out_dir.endswith('/') :
    out_dir = out_dir + '/'
  
  if not os.path.exists(out_dir) :
    os.makedirs(out_dir)

  logDir = in_dir
  filenames = [{"filename":os.path.join(logDir, f), "directory":out_dir} for f in os.listdir(logDir) if os.path.isfile(os.path.join(logDir, f))]
 
#  startCalculation(filenames[0])
#  return
  pool = multiprocessing.Pool(multiprocessing.cpu_count())
  pool.map(startCalculation, filenames)
  pool.close()
  pool.join()

#  for filename in filenames:
#    startCalculation(filename)
#  startCalculation(filenames[0])
#  pool.map(startCalculation, [1, 2, 3])

if len(sys.argv) < 3:
  print ("python age.py <in_directory> <out_directory>")
else:
  calcOpFairness(sys.argv[1], sys.argv[2]) 
