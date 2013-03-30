import os;
import sys;
import math;

def calcDistribution(filename, outputFilename):
  content = open(filename, 'r').readlines()

  numOps = 10000
  array = [0] * 100
  index = 0
  total = 0

  for line in content:
    if line.startswith('-'):
      index = index + 1
      parts = line.split(' ')
      value = int(parts[1])
      thread = value / numOps
      if thread < 0 or thread >= 100:
        print "Invalid thread {id} in file {filename}".format(id = thread, filename = filename)
        return
      elif array[thread] != 0:
        oldIndex = array[thread]
        difference = index - oldIndex
        total = total + difference

      array[thread] = index

  average = total / float(index-2)

  array = [0] * 100
  index = 0
  total = 0

  for line in content:
    if line.startswith('-'):
      index = index + 1
      parts = line.split(' ')
      value = int(parts[1])
      thread = value / numOps
      if thread < 0 or thread >= 100:
        print "Invalid thread: " + str(thread)
      elif array[thread] != 0:
        oldIndex = array[thread]
        difference = float(index - oldIndex) - average
        total = total + (difference * difference)

      array[thread] = index

  variance = math.sqrt(total / (index - 1))
  output = open(outputFilename, 'w')
  output.write ("average {average} ; variance {variance}\n".format(
      average = average, variance = variance))
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
