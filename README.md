Overview
========

This tool is meant to fix files that are created on a vfat filesystem mounted
with an `iocharset` different from your `locale`.

Background
==========

In vfat filesystem long (not 8.3 length) filenames are stored as UTF-16. This
is radically different from Linux-native filesystems which are encoding
agnostic. Not quite agnostic, the Linux kernel hardcodes the 0x2f byte to mean
path separator and 0x00 to mean terminator.

To let the user throw encodings at vfat thar are other than UTF-16, the vfat
implementation in Linux provides the `iocharset` mount option.  As the name
suggests, the `iocharset` is the charset from which filenames are converted
into UTF-16 on write and to which they are converted on read. Note that other
implementations (e.g. in Rockbox) are may not provide such conveniences and
simply interpret bytes as UTF-16.

The filename byte-sequences that are thrown at the filesystem originate from
the userspace, When userpace, e.g. the terminal, converts characters to bytes,
it obeys the locale. This means that if your locale is UTF-8, then the
byte-sequences that are going to be passed to the vfat are going to be UTF-8
sequences.

However, vfat interprets the bytes given to it as a sequence in the
`iocharset` encoding. (Remeber, it needs to interpret them because it needs to
re-encode them into UTF-16 for internal storage). Hence, *always choose
`iocharset` to equal your locale*. If you had it mismatched, then the filename
bytes will be re-encoded to UTF-16 according to the wrong source encoding.
The conversion will produce garbage, but nothing will appear to be wrong,
since the reverse conversion produces the original bytes.  However, things
will break as soon as you later mount with a different `iocharset` or mount on
an OS which operates with UTF-16 directly.

To summarise the following chain breaks if `locale != iocharset`:

              terminal                         kernel
    chars ---------------------> bytes -------------------------> data in vfat
            (chars -> [locale])           ([iocharset] -> UTF-16)

                       kernel                        terminal
    data in vfat ---------------------> bytes ------------------------> chars
                 (UTF-16 -> [iocharset])       ([locale] -> chars)           

Use case
========

Filenames that were created while e.g. `iocharset=iso8859-1` and UTF-8 locale
can be fixed by mounting the filesystem with `iocharset=utf8` and using this
tool.

The broken filename got created by

                terminal                kernel
    filename ---------------> bytes --------------------> garbage in vfat
              chars -> UTF-8         iso8859-1 -> UTF-16

the tool reverses this (after first undoing kernel's conversion on read):

                        kernel                fixenc: step 1
    garbage in vfat ----------------> bytes --------------------> ...
                     UTF-16 -> UTF-8           UTF-8 -> UTF-16           
 
            fixenc: step 2                    
    ... -----------------------> UTF-8 bytes of filename  
         UTF-16 -> iso8859-1              

Install and Usage
=================

First install [libiconv](http://www.gnu.org/software/libiconv/). Then,

    $ make 
    $ ./fixenc /path/to/dir/on/vfat

The directory will be crawled in bread-first order and all files renamed.

Use `-d` flag to do a dry-run and print the filenames before actual renames
(note that it doesn't descend into unrenamed directories).

lsdir
=====

Hexdumps a directory:

    $ ./lsdir / | head -5
    2e 2e 
    73 65 6c 69 6e 75 78 
    62 6f 6f 74 
    63 6f 72 65 
    76 61 72 

Credits
=======

iconv interfacing adapted from:
http://www.lemoda.net/c/iconv-example/iconv-example.html
