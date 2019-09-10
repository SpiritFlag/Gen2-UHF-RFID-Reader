import os
import sys
import matplotlib.pyplot as plt
import numpy as np

def plot_iq(ifile, qfile, num_lines, start_idx):

    ir = open(ifile, 'r')
    qr = open(qfile, 'r')

    for _ in range(start_idx):
        ir.readline()
        qr.readline()
    for _ in range(num_lines):
        iline = ir.readline()
        qline = qr.readline()
        ival = iline.split(' ')
        qval = qline.split(' ')
        try:
            ival.remove('\n')
            qval.remove('\n')
        except Exception as ex:
            print(ex)
        ival = [float(i) for i in ival]
        qval = [float(q) for q in qval]
        imin = np.min(ival)
        imax = np.max(ival)
        qmin = np.min(qval)
        qmax = np.max(qval)
        irange = (imax-imin)/2
        qrange = (qmax-qmin)/2
        imid = (imax+imin)/2
        qmid = (qmax+qmin)/2
        if irange > qrange:
            plt.xlim([imin,imax])
            plt.ylim([qmid - irange, qmid + irange])
        else:
            plt.xlim([imid - qrange, imid + qrange])
            plt.ylim([qmin, qmax])
        plt.scatter(ival, qval)
        plt.show()

    ir.close()
    qr.close()

if __name__ == "__main__":

    IFILE = sys.argv[1]
    QFILE = sys.argv[2]
    NUM_LINES = int(sys.argv[3])
    START_IDX = int(sys.argv[4])

    plot_iq(IFILE, QFILE, NUM_LINES, START_IDX)
