mkdir exp_log/$1
mv -i databit exp_log/$1/databit_$2
mv -i sample exp_log/$1/sample_$2
mv -i sample_I exp_log/$1/Isample_$2
mv -i sample_Q exp_log/$1/Qsample_$2
mv -i decoded_bit exp_log/$1/numbit_$2
mv -i graph.png exp_log/$1/graph_$2.png
