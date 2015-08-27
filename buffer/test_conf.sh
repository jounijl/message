
#
# cbsearch: Values are not read correctly if double delimiters are used.
#

cat tests/conf.txt | ./cbsearch -z -c 1 -b 1024 -l 512 -t -s "teksti.aihe2.teksti2.aiheen_t2"
cat tests/conf.txt | ./cbsearch -z -c 1 -b 1024 -l 512 -t -s "teksti.aihe5"

