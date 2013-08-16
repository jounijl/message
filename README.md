
Buffer to read and write streams. 

Reader reads valuepairs location in a buffer to a linked list. At least 
three differend methods are used. Methods can be compiled with macro 
defines in different object files.

UCS encoding is used (4 bytes) both in flow control characters and in the 
value names. Possible transfer encodings are: UTF-8, UTF-16 and UTF-32.

1-byte, 2-byte and 4-byte transfer encodings can be used. Adding an encoding is 
simple.

Testing program test_cb converts between encodings and tests input files 
value-name pairs.

<a href="http://jounijl.github.com/message">http://jounijl.github.com/message</a>
