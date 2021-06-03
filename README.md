## BLU-fx

#### License Information:
GNU General Public License v2.0

#### Fork status, contributing changes, etc.:
This is a fork from the last known state of the original repository, <github.com/bwRavencl/BLU-fx>.

The author of this fork is Steven L. Goldberg, who attempted to contribute these changes back
to the core project, however the Pull Request was denied due to the author claiming to have
abandoned the effort.  So, others are welcome to pick up from here and run with it if desired.

Therefore, please fork *this* repository instead of the original, and send PRs to `slgoldberg`.

Thanks!

## Building Blu-FX:
There are now multiple ways to build Blu-FX -- some are better than others, some don't work at
all currently, and some are just fine but haven't been updated. :-)

---
#### __Option 1: Docker + CMake (Mac, Linux, Windows)__:
---

With the Docker + CMake solution, you can build all three platforms in one command:
```
% (cd docker; make win lin mac)
```

See the README.md file in the docker subdirectory for more.

##### WARNING:
###### Unfortunately, the Mac version doesn't seem to load correctly yet (although it seems to build just fine), and the Windows binary is massively large compared to building natively. (Note: the Windows build has also not been tested as of this writing. :-) )

---
#### __Option 2: Makefile to build locally (Mac, Linux)__:
---
Linux users can simply type:
```
% make
```
to build for Linux on a Linux machine that's set up for development.

Mac users can with the development tools installed can similarly build using a Makefile,
however, the specific Makefile is different, so you must type its name, i.e.:
```
% make -f Makefile.mac
```

Either of these methods, when built locally, will put the binary in the "build" subfolder.
(Whereas, by convention, the Docker builds above will use "build-{win|lin|mac}", accordingly.)

---
#### __Option 3: Xcode to build locally (Mac)__:
---
There is an older Xcode project in this directory, so you can simply open the root in Xcode
to get started. However, there are probably many caveats with Xcode. It does build, per changes
made at the start of this fork. However, it hasn't been tested in a while, since the focus has
been on getting Docker and the Makefile method to work.

---
#### __Option 4: MS Visual Studio C++ to build locally (Win)__:
---
This seems to be supported, but honeslty I (Steve) have never looked at this.  So far my focus
has been on the Mac version.  Others are welcome to improve this, though it'd be ideal if we
can instead make the Docker version work better, i.e. streamline it to strip symbols and
reduce the total size which is currently exhorbitant (see above) and has not been verified.

## Caveats:

This version is in transition to support Docker and the new X-Plane plugin naming scheme.
So, for now anyway, some things may not work exactly the same way, depending on which build
method you use (see Building above).