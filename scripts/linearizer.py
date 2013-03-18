import os;
import multiprocessing;
import subprocess;
import ah_config;
import sys;
def getLineCount(filename):
  if not os.path.exists(filename):
    return -1

  logfile = open(filename, 'r')

  lines = 0

  for line in logfile:
    lines += 1

  return lines


def startCalculation(inputData):

  item = inputData["filename"]
  filename = ""
  lines = getLineCount(item)
  if lines > 0:

    filename = "{directory}{filename}".format(filename = os.path.basename(item), directory = inputData["directory"])
    if os.path.exists(filename) and os.path.getsize(filename) > 0:
      return
  else :
    print ("File {filename} is empty or does not exist.".format(filename=item))
    return

  resultFile = open(filename, 'wb')
  print ("{item}; {resultfile}".format(item = item, resultfile = filename))
  analyzer = subprocess.Popen(["../cpp/analyzer/linearizer", "sum", str(lines), item], stdout=resultFile)
  analyzer.wait()
  # resultFile.close()
  if analyzer.returncode and analyzer.returncode != 0 :
    print ("The analyzer failed for the file {filename}".format(filename = item))
    return

def calcLinearization(inputDir, outputDir, mode):
 
  if not inputDir.endswith('/') :
    inputDir = inputDir + '/'

  if not outputDir.endswith('/') :
    outputDir = outputDir + '/'

  if not os.path.exists(outputDir) :
    os.makedirs(outputDir)

  filenames = [{"filename":os.path.join(inputDir, f), "directory":outputDir, "mode":mode} for f in os.listdir(inputDir) if os.path.isfile(os.path.join(inputDir, f))]

  pool = multiprocessing.Pool(multiprocessing.cpu_count())
  pool.map(startCalculation, filenames)
  pool.close()
  pool.join()

#  for filename in filenames:
#    startCalculation(filename)
#  startCalculation(filenames[0])
#  pool.map(startCalculation, [1, 2, 3])

if len(sys.argv) < 2:
  print ("python op_fairness.py <directory>, e.g. 'python op_fairness pldi13/' for logfiles in /mnt/local_homes/ahaas/pldi13/")
else:
  calcOpFairness(sys.argv[1]) 
