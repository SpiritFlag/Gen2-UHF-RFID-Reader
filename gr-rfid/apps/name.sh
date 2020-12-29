mkdir ~/exp_log/$1

mv -i RN ~/exp_log/$1/RN
mv -i label ~/exp_log/$1/label
mv -i abs_signal ~/exp_log/$1/abs_signal
mv -i Isignal ~/exp_log/$1/Isignal
mv -i Qsignal ~/exp_log/$1/Qsignal

#cp -i ../misc/data/matched_filter ~/exp_log/$1/matched_filter
#cp -i ../misc/data/gate ~/exp_log/$1/gate

#mv -i RN ~/exp_log/$1/RN_$2
#mv -i label ~/exp_log/$1/label_$2
#mv -i abs_signal ~/exp_log/$1/abs_signal_$2
#mv -i Isignal ~/exp_log/$1/Isignal_$2
#mv -i Qsignal ~/exp_log/$1/Qsignal_$2
