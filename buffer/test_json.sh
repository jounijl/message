
#cat tests/text.json | ./cbsearch -c 4 -b 2048 -l 512 -J -s "\"widget\".\"window\".\"name\"" 2>&1
#cat tests/text.json | ./cbsearch -c 4 -b 2048 -l 512 -J -s "\"widget\".\"text\".\"alignment\"" 2>&1
#cat tests/text.json | ./cbsearch -c 4 -b 2048 -l 512 -J -s "\"widget\".\"window\".\"title\"" 2>&1

# Another file
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 4 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"title\"" 2>&1

# Array
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 4 -b 2048 -l 512 -J -s \
"\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossDef\".\"GlossSeeAlso\"" 2>&1

cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 4 -b 2048 -l 512 -J -s \
"\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossDef\".\"para\"" 2>&1

# After array, tree sloping downwards
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 4 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossSee\"" 2>&1


