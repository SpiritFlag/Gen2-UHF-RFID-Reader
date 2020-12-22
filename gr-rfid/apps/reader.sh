#! /bin/sh
rm -r debug_data
mkdir debug_data

cd debug_data
mkdir log
mkdir RN16_input EPC_input
mkdir RN16_preamble EPC_preamble
mkdir RN16_sample EPC_sample
mkdir gate
cd ../

rm log result RN label abs_signal Isignal Qsignal
python reader.py
cat result

python3 rule_based_decoding/main.py $1 50 3
