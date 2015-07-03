#
# Tests every object, string and an array
#

#{
#    "glossary": {
#        "title": "example glossary",
#		"GlossDiv": {
#            		"title": "S",
#			"GlossList": {
#                		"GlossEntry": {
#               		    	"ID": "SGML",
#					"SortAs": "SGML",
#					"GlossTerm": "Standard Generalized Markup Language",
#					"Acronym": "SGML",
#					"Abbrev": "ISO 8879:1986",
#					"GlossDef": {
#        	                		"para": "A meta-markup language, used to create markup languages such as DocBook.",
#						"GlossSeeAlso":
#						"GlossSeeAlso": ["GML", "XML"]
#	                 	   		},
#					"GlossSee": "markup"
#                		}}}}}} ...

# Object
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\"" 2>&1
# String
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"title\"" 2>&1
# String ERROR1
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"title2\"" 2>&1
# String ERROR2
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"title3\"" 2>&1
# Object
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\"" 2>&1
# String
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"title\"" 2>&1
# Object
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\"" 2>&1
# Strings
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\"" 2>&1
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"ID\"" 2>&1

# String ERROR3
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv2\".\"title\"" 2>&1

cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"SortAs\"" 2>&1
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossTerm\"" 2>&1

# Object
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv2\"" 2>&1

cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"Acronym\"" 2>&1
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"Abbrev\"" 2>&1
# Object
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossDef\"" 2>&1
# String
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossDef\".\"para\"" 2>&1
# Array
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossDef\".\"GlossSeeAlso\"" 2>&1
# String after downward slope '}'
cat tests/json.txt | ./cbsearch.CBSETSTATELESS -c 1 -b 2048 -l 512 -J -s "\"glossary\".\"GlossDiv\".\"GlossList\".\"GlossEntry\".\"GlossSee\"" 2>&1

