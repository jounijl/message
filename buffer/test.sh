
# Wikipedia: 
#Koodaus 	??????#Unicode 	U+00C5 	U+00E5 	U+00C4 	U+00E4 	U+00D6 	U+00F6

LANG=fi_FI.UTF-8
export LANG

# Word count
# wc -c ./tests/testi.txt

#
# Searches word 'unknown' in tests/testi.txt.
#

#
# Any 1-byte to all encodings
#
#./test_cb 1 1024 4096 tests/testi.txt
# CR:s
#./test_cb 1 1024 4096 tests/testi2.txt
# cat -e tests/testi2.txt # display non printing characters
./test_cb 1 512 1024 tests/testi2.txt

echo; file tests/*.out

#
# Stress test
# 
# ./tests/loop ./tests/testi.txt | ./test_cb 1 1024 4096 -
# bash: return value 137 is signal 9.

#
# Test sequential name search
#
# cat tests/testi.txt | ./cbsearch -c 4 -b 2048 -l 512 unknown
# cat tests/testi.txt | ./cbsearch -c 4 -b 1028 -l 128 -s "unknown nimi1 viides"
# <CR><LF> (echo_CR.sh echo_LF.sh add_cr_to_file.sh)
# cat tests/testi2.txt | ./cbsearch -c 4 -b 1028 -l 128 -s "unknown nimi1 viides"

#
# Convert between encodings
#
# cat tests/testi.txt | ./cbconv -i 1 > test_output.txt

#
# Test cb_ring.c
#
# cat tests/testi.txt | ./cbfifo 2>>/dev/null
# cat tests/testi.txt | ./cbfifo | wc -c

exit 0

#
# UTF-8 to all encodings
#
./test_cb 3 1024 4096 tests/testi_utf-8.txt

exit 0

#
# Autodetection to all encodings
#
./test_cb 0 1024 4096 tests/testi_utf-8.txt
