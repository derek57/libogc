# libOGC is actually RTEMS source code

## RTEMS branch

This is what "libOGC" would have looked like if shagkur and Dave Murphy (a. k. a. WinterMute / WntrMute)
hadn't chosen to rename and / or remove:

  - function names
  - function members
  - variable names
  - preprocessor definitions
  - original source code file headers
  - file names
  - whitespace

This repository (branch) basically is "libOGC" as of v1.8.12 without all those changes mentioned above and
the file headers restored as well after some Google search turned out that there are code similarities
between "libOGC" and some older RTEMS commits which then finally made things clear:

At least half of the "libOGC" source code is actually RTEMS source code - nothing more, nothing less
(without itself giving any credits to the original developers due to removing the original file headers).

How disgusting...
