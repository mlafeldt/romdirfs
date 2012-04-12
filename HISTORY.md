History
=======

v1.2 (Apr 12 2012)
------------------

* Migrate project from hg to git.
* Add Makefile.
* Move source files to root folder.
* Let `romdir_extract()` return number of bytes written. Fixes a compile error
  under gcc 4.6.1.
* Support Mac OS X.
* Add Travis CI config.
* Update documentation and convert it to markdown.

v1.1 (Aug 25 2009)
------------------

* Read whole input file into memory rather than reading each module's data.
* Define `-DUSE_MMAP` to map the input file into memory with `mmap()`.
* Add `EXTINFO` information to `romfile_t` struct.

v1.0 (Jul 15 2009)
------------------

* Initial public release
