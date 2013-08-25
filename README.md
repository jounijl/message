Buffer to read and write streams.
=================================

Reader reads valuepairs location in a buffer to a linked list. At least three 
differend methods are used. Methods can be compiled with macro defines in 
different object files.


Encodings:
----------

UCS encoding is used (4 bytes) both in flow control characters and in the value 
names. Possible transfer encodings are: UTF-8, UTF-16 and UTF-32.

1-byte, 2-byte and 4-byte transfer encodings can be used. Adding an encoding is 
possible.


Search methods:
---------------

At least two search methods can be used. Either unique names in stream (2) or 
multiple same names in stream (1). To search the same name again, it's possible to 
set the size of the buffer small enough to not to remember the names in list 
structure (2) to find the same names again or set to multiple same names to search 
the next same names in buffer also (1).

CBFILE can be used as a stream (primary use), a file or a buffer.


Testing:
--------

Testing program 'test_cb' converts between encodings and tests input files 
value-name pairs.

Testing program 'cbsearch' searches a name or names from input. It finds multiple 
same names from stream.


<a href="http://jounijl.github.io/message">http://jounijl.github.io/message</a>
