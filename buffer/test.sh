
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

# valgrind --leak-check=full --show-leak-kinds=all -v --track-origins=yes ./test_cb 1 512 1024 tests/testi2.txt

echo; file tests/*.out

#echo
#echo "Repeating to all files"
#sleep 2

# file tests/*.out | cut -d':' -f1 | while read I

#
# Stress test
# 
# ./tests/loop ./tests/testi.txt | ./test_cb 1 1024 4096 -
# bash: return value 137 is signal 9.

#
# Name search
#
# cat tests/testi.txt | ./cbsearch -c 4 -b 2048 -l 512 unknown
# cat tests/testi.txt | ./cbsearch -c 4 -b 1028 -l 128 -s "unknown nimi1 viides"
# <CR><LF> (echo_CR.sh echo_LF.sh add_cr_to_file.sh)
# cat tests/testi2.txt | ./cbsearch -c 4 -b 1028 -l 128 -s "unknown nimi1 viides"
# cat tests/testi2.txt.utf8 | ./cbsearch -i 3 -c 4 -b 1028 -l 128 -s "unknown nimi1 viides"
#
# Name from tree:
# cat tests/testi.txt | ./cbsearch.CBSETSTATELESS -c 4 -b 2048 -l 512 -t -s "nimi1 bb.ff.gg unknown" 2>&1 | more
# cat tests/testi.txt | ./cbsearch.CBSETSTATELESS -c 4 -b 2048 -l 512 -t -s "bb.ff.gg nimi1 unknown" 2>&1 | more

#for I in CBSETSTATELESS CBSETSTATEFUL CBSETSTATETOPOLOGY
#  do
#    cat tests/testi.txt | ./cbsearch.$I -c 4 -b 2048 -l 512 unknown 2>&1 
# done | less

# Subsearch
## cat tests/testi2.txt.utf8 | ./cbsearch -i 3 -c 4 -b 1028 -l 128 -t -s "unknown nimi1 viides"

# Regular expressions
# LD_LIBRARY_PATH=path_to_pcre32_with_utf:$LD_LIBRARY_PATH; export LD_LIBRARY_PATH
# cat tests/testi2.txt.utf8 | ./cbsearch -x unknow[n]
# cat ~/Documents/Pyora/pyora_specialized.txt | ./test_regexp_search "Wheel" 2>&1 | grep -v 24
# cat ~/Documents/Pyora/pyora_specialized.txt | ./test_regexp_search "Whe.l" 2>&1 | grep -v 24
# cat ~/Documents/Pyora/pyora_specialized.txt | ./test2_regexp_search "(*UTF8)Wheel" 2>&1 | grep -v 24
# cat ~/Documents/Pyora/pyora_specialized.txt | pcregrep "Wheel"
# cat ~/Documents/Pyora/pyora_specialized.txt | pcregrep "Whe.l"
# ../../pcre/bin/pcretest -32 -C pcre32 
## less ./test-suite.log
## man pcrestack
## ulimit -s

#
# Convert between encodings
#
# cat tests/testi.txt | ./cbconv -i 1 -o 3 > test_output.txt
#
# Encoding numbers: cat ../include/cb_encoding.h | grep CBENC

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
