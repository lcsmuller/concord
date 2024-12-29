# KNOWN BUGS

This file outlines the known bugs that are yet to be fixed

## Solaris
- Concord does not work on Solaris versions prior to 9, due to dependance
on newer C library functions. 

## z/OS
- If you use the IBM Metal C/C++ compiler, you need to force the usage of
the ASCII character set. No Makefile parameters or documentation notes
for this have been made.
