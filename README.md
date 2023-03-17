# shebang #!

Scripts usually have a `#!` at the very start of the first line, known as *shebang*. 
This line identifies the interpreter for executing a script.

It has existed for a long time, but in current OSes it's of quiet limited use.
Often ot can only describe an absolute path to an interpreter and a single parameter.

This tool changes that. It allows searching the interpreter in `$PATH` like a shell would,
allows for multiple parameters, and allows for very basic `\` escapes. (Currently, the escapes
are only useful for escaping spaces and backslashes. Things like `\n` are the same as `n`).

To build this program, just type `make`.

To enable/disable this program, simply execute `shebang --enable` or `shebang --disable` respectively.
This registers itself as a `binfmt_misc` interpreter for files starting with `#!`.
This requires `/proc/sys/fs/binfmt_misc` to be mounted already.

It would be preferable if the kernel could do this on it's own, executing a program every time just for this is a bit of a waste.
But since that's currently not possible, I recommend registring it as soon on startup as possible, ideally in the initramfs.
