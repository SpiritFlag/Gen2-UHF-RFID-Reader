# -*- coding: utf-8 -*-

from tqdm import tqdm
import math
import time
import os

num_half_bit = 25  # half_bit의 sample 수
bit_preamble = 6  # preamble의 bit 수
bit_data = 128  # data의 bit 수
bit_extra = 12  # preamble, data를 제외하고 앞뒤 여백으로 포함된 sample의 bit 수

class Signal:
  file_name = ""
  answer = ""
  samples = list()
  std_samples = list()

  def __init__(self, file_name, line1, data):
    self.file_name = file_name
    self.answer = [int(line1[i]) for i in range(0, len(line1)-1)]
    self.samples = list()
    for idx in range(0, len(data)-1):  # -1은 제일 뒤의 '\n'을 제거해주기 위함
      self.samples.append(data[idx])
    self.samples = [float(i) for i in self.samples]

    if(len(self.samples) != 0):
      avg = 0
      for sample in self.samples:
        avg += sample
      avg /= len(self.samples)

      std = 0
      for sample in self.samples:
        std += pow(sample - avg, 2)
      std /= len(self.samples)
      std = math.sqrt(std)

      self.std_samples = [((i - avg) / std) for i in self.samples]

      for idx in range(len(self.std_samples)):
        if self.std_samples[idx] > 1:
          self.std_samples[idx] = 1
        if self.std_samples[idx] < -1:
          self.std_samples[idx] = -1

def read_file(size):
  file1 = open("databit", "r")
  file2 = open("sample", "r")

  signals = list()

  for n in tqdm(range(size), desc="READING", ncols=80, unit="signal"):
    line1 = file1.readline()
    if not line1: break
    line2 = file2.readline()
    if not line2: break
    data = line2.split(" ")
    signals.append(Signal("a", line1, data))

  return signals

def detect_preamble(signal):
  # preamble mask
  mask = [1.0] * 2 * num_half_bit  # 1
  mask += [-1.0] * num_half_bit  # 2
  mask += [1.0] * num_half_bit
  mask += [-1.0] * 2 * num_half_bit  # 3
  mask += [1.0] * num_half_bit  # 4
  mask += [-1.0] * num_half_bit
  mask += [-1.0] * 2 * num_half_bit  # 5
  mask += [1.0] * 2 * num_half_bit  # 6

  mask2 = list(mask)
  for idx in range(len(mask2)):
    mask2[idx] *= -1.0

  max_idx = 0
  max_score = 0
  state = 0

  for idx in range(bit_extra * 2 * num_half_bit):
    score = 0
    score2 = 0
    for mask_idx in range(len(mask)):
      score += mask[mask_idx] * signal.std_samples[idx+mask_idx]
      score2 += mask2[mask_idx] * signal.std_samples[idx+mask_idx]
    if score > max_score:
      max_idx = idx
      max_score = score
      state = -1
    if score2 > max_score:
      max_idx = idx
      max_score = score2
      state = 1
    #print(idx, score, score2)

  #print(max_idx + bit_preamble * 2 * num_half_bit, state)
  return max_idx + bit_preamble * 2 * num_half_bit, state  # data가 시작되는 sample의 index를 반환



def detect_data(signal):
  mask0L = [1.0] * num_half_bit
  mask0L += [-1.0] * num_half_bit
  mask0L += [1.0] * num_half_bit
  mask0L += [-1.0] * num_half_bit

  mask0H = [-1.0] * num_half_bit
  mask0H += [1.0] * num_half_bit
  mask0H += [-1.0] * num_half_bit
  mask0H += [1.0] * num_half_bit

  mask1L = [1.0] * num_half_bit
  mask1L += [-1.0] * 2 * num_half_bit
  mask1L += [1.0] * num_half_bit

  mask1H = [-1.0] * num_half_bit
  mask1H += [1.0] * 2 * num_half_bit
  mask1H += [-1.0] * num_half_bit

  #state = 0  # preamble이 high level로 끝나기 때문에 첫 bit는 low level로 시작
  idx, state = detect_preamble(signal)
  shift = [-3, -2, -1, 0, 1, 2, 3]  # 디코딩 성공률이 떨어질 경우 shift 범위를 넓힐 수 있음
  cur_shift = 0
  success = 0

  for num_bits in range(bit_data):
    if state == 1:
      mask0 = mask0H
      mask1 = mask1H
    else:
      mask0 = mask0L
      mask1 = mask1L

    max_score = 0
    max_value = -1

    for s in shift:
      score0 = 0
      score1 = 0

      for mask_idx in range(4 * num_half_bit):  # 앞뒤로 half bit씩 확장한 mask와 비교하기 때문에 index에서 num_half_bit을 빼주는 것에 주의
        if idx-num_half_bit+mask_idx+s >= len(signal.std_samples):
          continue
        score0 += mask0[mask_idx] * signal.std_samples[idx-num_half_bit+mask_idx+s]
        score1 += mask1[mask_idx] * signal.std_samples[idx-num_half_bit+mask_idx+s]

      if score0 > max_score:
        max_score = score0
        max_value = 0
        cur_shift = s
      if score1 > max_score:
        max_score = score1
        max_value = 1
        cur_shift = s
      #print(num_bits, idx, cur_shift, score0, score1, signal.answer[num_bits])

    # 틀린 bit가 나오면 곧바로 종료할 때는 이곳을 주석해제하여 사용
    '''
      if max_value == 1:
        state *= -1  # bit가 1인 경우에만 다음 bit의 시작이 반전
        if signal.answer[num_bits] != 1:
          return False
      else:
        if signal.answer[num_bits] != 0:
          return False

      idx += 2 * num_half_bit + cur_shift  # 보정된 shift는 다음 bit 해독에도 반영

    return True
    '''

    # 틀린 bit가 나와도 끝가지 디코딩하여 성공한 개수를 셀 때는 이곳을 주석해제하여 사용
    if max_value == 1:
      state *= -1  # bit가 1인 경우에만 다음 bit의 시작이 반전
      if signal.answer[num_bits] == 1:
        success += 1
    else:
      if signal.answer[num_bits] == 0:
        success += 1

    idx += 2 * num_half_bit + cur_shift  # 보정된 shift는 다음 bit 해독에도 반영

  return success

import matplotlib.pyplot as plt
import random
iteration = 500
pickup = 100

signals = read_file(iteration)
decoded_bit = [0] * (bit_data+1)

rn_index = list()
for idx in range(0, pickup):
  rn = random.randint(0, iteration-1)
  while rn in rn_index:
    rn = random.randint(0, iteration-1)
  rn_index.append(rn)
rn_index.sort()
#print(rn_index)

count = 0
for idx in tqdm(range(len(rn_index)), desc="DECODING", ncols=80, unit="signal"):
  if(len(signals[rn_index[idx]].samples) == 0):
    continue
  decoded_bit[detect_data(signals[rn_index[idx]])] += 1
  count += 1

fw = open("numbit", "w")
for d in decoded_bit:
  fw.write(str(d) + "\n")

print(str(decoded_bit[128]) + " / " + str(count))
plt.plot(decoded_bit)
plt.show()
plt.plot(decoded_bit)
plt.savefig("graph.png", dpi=1000)
plt.clf()
