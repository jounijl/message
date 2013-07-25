
# Wikipedia: 
#Koodaus 	Å	å	Ä	ä	Ö	ö
#Unicode 	U+00C5 	U+00E5 	U+00C4 	U+00E4 	U+00D6 	U+00F6

LANG=fi_FI.UTF-8
export LANG

# UTF-8 to all encodings
#./test_cb 0 1024 4096 tests/testi_utf-8.txt

# Any 1-byte to all encodings
./test_cb 1 1024 4096 tests/testi.txt

