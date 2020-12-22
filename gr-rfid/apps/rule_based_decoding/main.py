import sys
import timeit

from tqdm import tqdm
from global_vars import *
from read_set import *
from IQconvert import *
from decode_data import *



if __name__ == "__main__":
  try:
    n_signal = int(sys.argv[1])
    constant_bit_len = float(sys.argv[2])
    n_shift = int(sys.argv[3])

    print("\n\n\t*** RULE BASED DECODING ***")

    Isignal, Qsignal, label = read_set(n_signal)
    signal = IQconvert(Isignal, Qsignal, len(label))

    success = 0

    for idx in tqdm(range(len(label)), desc="TESTING", ncols=100, unit=" signal"):
      fail = False
      error_idx = -1
      n_error = 0

      predict = decode_data(signal[idx], constant_bit_len, n_shift)

      for n in range(n_bit_data):
        if predict[n] != label[idx][n]:
          if fail is False:
            error_idx = n
            fail = True
          n_error += 1

      if fail is False:
        success += 1

    print(f"\n\tRESULT=\t{success:4d} / {len(label):4d} ({100*success/len(label):6.2f}%)\n\n")

  except Exception as ex:
    _, _, tb = sys.exc_info()
    print("[main:" + str(tb.tb_lineno) + "] " + str(ex) + "\n\n")
