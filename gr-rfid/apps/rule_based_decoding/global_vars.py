data_path = ""
Isignal_path = data_path + "Isignal"
Qsignal_path = data_path + "Qsignal"
label_path = data_path + "label"



n_sample = 7300
n_cw = 50
n_bit = 50
n_half_bit = int(n_bit / 2)
n_bit_preamble = 6
n_bit_data = 128
n_extra = int(n_sample - n_bit * (n_bit_preamble + n_bit_data))
