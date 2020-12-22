import sys
import numpy as np

from tqdm import tqdm
from global_vars import *



def read_set(n_signal):
  try:
    fileI = open(Isignal_path, "r")
    fileQ = open(Qsignal_path, "r")
    fileL = open(label_path, "r")

    Isignal = []
    Qsignal = []
    label = []

    for idx in tqdm(range(n_signal), desc="READING", ncols=100, unit=" signal"):
      lineI = fileI.readline().rstrip(" \n").split(" ")
      lineQ = fileQ.readline().rstrip(" \n").split(" ")
      lineL = fileL.readline().rstrip("\n")

      if len(lineI) > 1:
        Isignal.append([float(i) for i in lineI])
        Qsignal.append([float(i) for i in lineQ])
        label.append([int(i) for i in lineL])

    fileI.close()
    fileQ.close()
    fileL.close()

    return np.array(Isignal), np.array(Qsignal), np.array(label)

  except Exception as ex:
    _, _, tb = sys.exc_info()
    print("[read_set:" + str(tb.tb_lineno) + "] " + str(ex) + "\n\n")
