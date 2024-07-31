## BLU-fx

#### License Information:
GNU General Public License v2.0

#### Fork status, contributing changes, etc.:
This is the new home of BLU-FX, a plugin for X-Plane that provides a very simple way for users
(on Mac, Windows, and Linux!) to customize a set of basic graphics filters, such as brightness,
contrast, color balance, and more.

While this repository is shown as a fork of the original repository <github.com/bwRavencl/BLU-fx>.
The owner of that repository has given full support to the continuation of this project from this
repository going forward. (Again, the original repository is now just an archive.)

The owner of this new fork is Steven L. Goldberg, aka "brat", aka "slgoldberg" here and on the
X-Plane "dot org" forums.

Therefore, please fork *this* repository instead of the original, and send PRs to `slgoldberg`.
(Note: before forking this version, please contact the owner, since the next set of changes
will be quite large, including a complete rewrite of the user interface using ImGui and ImgWindow,
which will change about 80% of the code as of this writing, and which will obviate many of the
known issues (bugs) with the current slider behavior, most of which are simply artifacts of a
very old X-Plane library that is barely able to keep up with modern X-Plane UI.

Note that this fork ONLY supports versions of X-Plane starting with v11.10, going through the
latest known versions (as of this writing, X-Plane v12).  Also, the build instructions below
are not quite up to date, as most things are now done in Docker, except the Mac build is
currently only tested using the direct xCode build method from a macOS machine. (The build
can be run either on an Apple Silicon or on an Intel-based Mac -- and either will produce
a universal binary that contains support for both target platforms in a single .xpl file.
For this to work, the only versions of macOS that are supported are 10.11 and later.)

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
