import sys
import numpy as np

from tqdm import tqdm
from global_vars import *
from kalman_filter import *



def IQconvert(Isignal, Qsignal, n_signal):
  try:
    signal_list = []

    for idx in tqdm(range(n_signal), desc="CONVERTING", ncols=100, unit=" signal"):
      signal = []
      Icenter, Qcenter = kalman_filter(Isignal[idx], Qsignal[idx])

      for x in range(n_sample):
        signal.append(np.sqrt((Isignal[idx][x] - Icenter) ** 2 + (Qsignal[idx][x] - Qcenter) ** 2))

      signal = np.array(signal)
      avg = np.mean(signal)
      std = np.std(signal)

      signal_std = []

      for x in range(len(signal)):
        sample = (signal[x] - avg) / std

        if sample > 1:
          signal_std.append(1)
        elif sample < -1:
          signal_std.append(-1)
        else:
          signal_std.append(sample)

      signal_list.append(signal_std)

    return np.array(signal_list)

  except Exception as ex:
    _, _, tb = sys.exc_info()
    print("[IQconvert:" + str(tb.tb_lineno) + "] " + str(ex) + "\n\n")
