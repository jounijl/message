
# Wikipedia: 
#Koodaus 	??????#Unicode 	U+00C5 	U+00E5 	U+00C4 	U+00E4 	U+00D6 	U+00F6

LANG=fi_FI.UTF-8
export LANG

# Word count
# wc -c ./tests/testi.txt

#
# Any 1-byte to all encodings
#
./test_cb 1 1024 4096 tests/testi.txt

file tests/*.out

exit 0

# Output
echo
echo "Output, original:"
cat tests/testi.txt
echo
echo "Output, encoding 0:"
cat tests/testi.txt.0.out
echo
echo "Output, encoding 1:"
cat tests/testi.txt.1.out
echo 


#
# UTF-8 to all encodings
#
./test_cb 3 1024 4096 tests/testi_utf-8.txt
# Output
echo
echo "Output, original:"
cat tests/testi_utf-8.txt
echo
echo "Output, encoding 0:"
cat tests/testi_utf-8.txt.0.out
echo
echo "Output, encoding 1:"
cat tests/testi_utf-8.txt.1.out
echo 

exit 0

#
# Autodetection to all encodings
#
./test_cb 0 1024 4096 tests/testi_utf-8.txt
# Output
echo
echo "Output, original:"
cat tests/testi.txt
echo
echo "Output, encoding 0:"
cat tests/testi.txt.0.out
echo
echo "Output, encoding 1:"
cat tests/testi.txt.1.out
echo 


