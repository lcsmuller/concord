# Running Concord on old systems

*This guide will show you how to run Concord on some really out-of-date machines, mainly old UNIX systems. This guide will assume
that you are already familiar with the operation of the old UNIX system you wish to run this on.*

## Sun Workstations and Servers -- Solaris: Sun Solaris 9 on SPARC or x86

For the most part, running Concord on old versions of Solaris is not too difficult. Compilation with GCC is trivial, but, somewhat
boring. For this guide, we will use the Sun C compiler; this compiler is derived from the UNIX System V Release 4.2 "acomp" compiler
(as it is named by the error messages, source code, and internal UNIX documentation). Now, this guide will already assume that you
have Sun WorkShop or Sun Studio installed (you *do* need a C99 compiler here, so, Sun Studio 11 from 2008 will do). 

Once you've gotten Sun Studio installed somewhere, presumably /opt/SUNWspro, add /opt/SUNWspro/bin to your path. Make a test 
program to verify that the compiler works, and compile the following programs, in this order:
- GNU Make 3.77+
- GNU Bash 2.4+
- OpenSSL 0.9+
- cURL 4.74.1+

Please note that you **must** compile cURL with some kind of SSL certificate directory, as it is always a bad idea to disable
certificate verification. Since you are probably running Solaris 9, you will definitely need updated SSL certificates. You can 
get these from NetBSD's pkgsrc project, search for a root certificates package. Once you have those installed to somewhere
suitable, like, `/etc/ssl/certificates`, configure cURL with that SSL certificate directory; don't forget to tell it to use 
OpenSSL. You will need to run the configure script with Bash, and build it using GNU Make. 

Once all of that is done, you can now build Concord. You *will* need to supply a different set of LDFLAGS, like so:
```
LDFLAGS=-L/usr/local/lib -ldiscord -lcurl -lpthread -lssl -lsocket -lcrypto -lnsl -lz -lrt
```
This is assuming that your OpenSSL and cURL libraries are in `/usr/local/lib`. If they aren't, supply the correct path. Once you
have made the edits to that Makefile, simply run `make` and verify that everything worked. Once it finishes, run `make install`
(as root) and you should now have a working Concord installation on Solaris, compiled with a compiler besides GCC. 

### Caveats and problems you might have

While Concord can compile under old Solaris systems using Sun Studio's compiler, you might need to make some adjustments to the 
source to get it to compile perfectly. The `gencodecs` system will be, for sure, the biggest source of trouble. While it can be
preprocessed right on Solaris, you might need to use the GNU C preprocessor. `cc -E` might not work (depending on your compiler
version), so, substitute in `cpp` for `cc -E` and try that too. **You are probably going to run into SSL problems.** Welcome to
old versions of OpenSSL; you might want to try the newest version you can. If all else fails, point it at a Fosscord instance, and
don't expose it to the open internet.



### DEC Alpha Systems -- OSF/1: DEC OSF/1 AXP, DEC DIGITAL UNIX, Compaq/HP Tru64 UNIX
For the most part, the instructions listed above will also apply here. The hard part is going to be finding and installing the
latest version of the compiler. Tru64 5.1B6 is the latest OS version, and the compiler that is included with the OS is fully
C99 compliant. Fortunately, the compiler for Tru64 5.1B works on 5.0, and, if you are very clever, 4.0G. Nonetheless, I recommend
that you compile a binary on 5.1 and run it on anything older if you must use an older OS version. 



### HP 9000 UNIX Systems -- HP-UX: HP-UX 11.11 on PA-RISC
HP aCC is supposed to be C99 compliant, but, you might run into problems. If you have a GCC toolchain available on your machine,
I highly recommend that you just use that, as aCC is slow and painful. Either way, you'll need to compile everything shown above,
especially OpenSSL.


### DEC VAX Systems -- NetBSD: NetBSD 9 on a MicroVAX, VAXstation, or VAX-11
*I am going to say right off the bat that you are not going to be able to do this on real hardware. The CPU is simply too slow
to run real-time SSL, and the machine you probably have likely does not have enough RAM. Doing this on real hardware will limit
you to having to use an external proxy (like retro-proxy) or forgo SSL entirely and use Fosscord.*

Assuming you are still determined to do this, this is probably the easiest system. Since Concord compiles fine and very easily
under NetBSD, all you need to do is install a binary package of (or compile) OpenSSL, cURL, and install the SSL certificates.
Once you've done that, simply run `make` and `make install`. 


### IBM RISC System/6000 (RS/6000) Workstations/Servers and POWER Series -- AIX: AIX 7.2 on a POWER8
For the most part, follow the Solaris installation instructions, but, make sure that you've got IBM XL C/C++ installed. It likes
to install itself in weird locations, like `/opt/IBM/xlc/16.0.0/bin/cc`. You will **definitely** want GNU Make, as AIX's ancient
UNIX SVR3 `make` is far too crippled to compile Concord. 


### IBM System/390 and System Z Mainframes -- z/OS: any Z system
The entirety of the compilation and running here happens under the UNIX environment (USS). Either SSH into your z/OS host, telnet
in on port 1023, or run OMVS from TSO to get at it.

As before, we are going to use the IBM-provided XL C/C++ compiler. If you follow the instructions for AIX, they are pretty much
100% applicable to z/OS. However, watch out for endianess conversion! Again, I am assuming that you are a skilled user of the 
z/OS UNIX System Services environment. When you extract a tar file, use `pax -rv -o to=IBM-1047 < file.tar` in order to convert
the character set over to EBCDIC. If you want to run it as a batch job, use the BPXBATCH program. This looks something like this:
```
//useridC JOB (1),CLASS=A,MSGCLASS=H,MSGLEVEL=(1,1)
//STEP1 EXEC PGM=BPXBATCH,REGION=128M,PARM='SH'
//STDIN DD PATH='/path/to/bot/startup/script.sh`,PATHOPTS=(ORDONLY)
//STDOUT DD SYSOUT=*
//STDERR DD SYSOUT=*
```


