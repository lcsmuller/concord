# Concord Frequently Answered Questions

Q. Why C?

A. C is certainly a very simple language, but, its simplicity causes a lot of work that would otherwise be done by the language 
or compiler in other languages to be thrown onto the user. Memory management is the most famous example -- C requires you to
manage your own memory. Complex tasks can take a lot of manual work, and, a Discord bot is no simple task. However, most of the
difficult work that Concord has to do is handled by the cURL library, libcurl. Since Discord is a HTTP API-based system, it
takes only a few HTTP requests to get authenticated against the Discord gateway, and a simple WebSocket emulation layer to
to send and receive events. Furthermore, Concord is an extremely portable library, and runs on many many more systems than the
usual Windows, Mac OS X, Linux, and BSD that most people know about. The true answer to the question is, why not?

Q. How is Concord asynchronous? I thought async programming was nigh impossible in C?

A. Basically, Concord internally maintains an event queue that is checked with every inbound request. When you "submit" an async
request to be processed by Concord, it is appended to that event queue. When an event comes in that "wakes up" that event queue,
all of the withstanding queued asynchronous events are checked for completion, and, events that are "ready to fire" are processed.
This design is relatively simple, and, external interrupts (such as those provided by the `alarm` and `setitimer` C library 
functions) can easily be used to drive an asynchronous event queue without relying on operating system-provided features (such as
`epoll`, `kqueue`s, event sockets, or I/O completion ports). 

Q. Why do I have to run `make clean` and `make` for every header change?

A. By default, large projects using traditional Makefiles do not automatically rebuild themselves when a header changes. This is
because of the principle of "changing a file that everything includes will result in a recompile of anything anyways."

Q. Is there any room for another build system, like CMake or Autotools?

A. Generally speaking, the maintainers of this library do not want to maintain multiple build systems, the current one works
excellently. 

Q. How does this deal with SSL/TLS, and API requests in general?

A. Rather than using OpenSSL directly (or any other SSL implementation, like CryptoAPI on Windows) to communicate with Discord and
make API requests manually, Concord wraps itself around cURL, a common library that makes dealing with things like this relatively
easy. cURL handles the SSL/TLS requirement by abstracting away implementation-differing details to a common interface, activated
by passing any `https`-containing URL to any of the cURL functions that starts a connection.


