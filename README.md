#####Buffer to read and write streams.

Reader reads valuepairs location in a buffer to a linked list. At least three 
differend methods are used. Methods can be compiled with macro defines in 
different object files.

###### Encodings

UCS encoding is used (4 bytes) both in flow control characters and in the value 
names. Possible transfer encodings are: UTF-8, UTF-16 and UTF-32. 

1-byte, 2-byte and 4-byte transfer encodings can be used. Adding an encoding is 
possible.

###### Search methods
 
At least two search methods can be used. Either unique names in stream or 
polysemantic names in stream (default).

###### File types

CBFILE can be used as a stream (primary use), a file or as a buffer. These are
minor changes in namelist size and in use of file descriptors.

###### Data

CBFILE can be set to use RFC 2822 unfolding and case insensitive names. 
It's possible to stop at RFC 2822 header. Other possibilities are listed in
cb_conf. It is possible for example to remove spaces, tabs and crlf characters 
between values and names. 

###### Testing programs
 
Testing program test_cb converts between encodings and tests input files 
value-name pairs.

Utility programs to make tests cases are found in directory buffer/tests. For 
example loop outputs file endlessly to output.

Testing program **cbsearch** searches a name or names from input. It finds multiple 
same names from stream.
 
Testing program **cbconv** converts between all encodings ( usage: flag -h )
including four byte UCS representation.

###### Introduction

 
<a href="http://jounijl.github.io/message">http://jounijl.github.io/message</a>

