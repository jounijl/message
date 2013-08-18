
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
./test_cb 1 1024 4096 tests/testi.txt

file tests/*.out

#
# Stress test
# 
#./tests/loop ./tests/testi.txt | ./test_cb 1 1024 4096 -
# bash: return value 137 is signal 9.

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
