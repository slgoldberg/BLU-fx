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
all currently, and some are just fine but haven't been updated. :-)  The first two depend on
you installing Visual Studio Code (VSCode) in order to build for macOS via CMake using the
macOS SDK, and/or to run the Docker cross-platform build (currently for Linux and Windows only).

---
#### __Option 1: (macOS only) Visual Studio Code (VSCode) + CMake__:
---
Currently, the only way to build a universal binary that supports both ARM and x86 builds for
macOS is to build _on_ a macOS machine (running macOS 12.12 or above). The good news is that
this same VSCode instance can be used to run the Docker builds for Windows and Linux, making
it easy to build for all three platforms in "one place", even though you need to use a different
method (currently) to build specifically for macOS:

Assuming you've properly configured VSCode to build for C++ using the xCode SDK that is available
for your macOS installation, you'll also need the CMake extension and other related extensions in
order to fully build the release for macOS (and you'll need some bits just to run the Docker
builds, below).

Make sure you're configured for production "Release" builds, and NOT for "Debug", and then simply
build for macOS by running CMake. The current build target "install" will copy the files to a
specific place for testing in X-Plane on your local machine; you can edit/change that as you like
in the `CMakeLists.txt` file.

To confirm that the resulting binary (`./build/blu_fx.xpl`) is set up as a universal binary and
thus will correctly support Apple Silicon **and** Intel in one image, you can run the "`lipo`"
command to verify that both are present:

```
% lipo -archs ./build/blu_fx.xpl
x86_64 arm64
```
If the output isn't "`x86_64 arm64`", but is just _one_ of those, then you may need to shake the
tree a little to get it to build the other one. It's funny that way. Once it starts working, it
works. But for some reason, sometimes not the first time.  So just keep shaking it. :-)  Eventually
it'll work. You may need to try different ways of tricking it into working. :-)  (No, I have no
idea why.)

---
#### __Option 2: VSCode + Docker + CMake (Linux, Windows, and -- in theory -- macOS)__:
---

With the Docker + CMake solution, you can build for at least the Windows and Linux platforms, but also the Mac (however, this will currently _only_ generate the binary for the x86_64 platform) using a single command:
```
% (cd docker; make win lin mac)
```

See the README.md file in the docker subdirectory for more.

##### WARNING:
###### Unfortunately, as of this writing, the Docker build for macOS will only build for x86_64, _not_ for Apple Silicon (arm_64). This will be addressed when we can replace the current macOS SDK in the Docker image with a newer one that supports universal binary development. Until then, in order to get the full support promised in the reelease notes, build for macOS using the VSCode method, below, from a macOS system running any version from 10.15 or higher (preferably higher!).

---
#### __Option 3: Makefile to build locally (Mac, Linux)__:
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
#### __Option 3: Xcode to build locally (macOS only)__:
---
The xCode project to build BLU-fx is now completely up to date, and works great -- in fact, I use this to develop and rapidly iterate, by doing Debug builds here, before I switch to building officially using method #1 above (on VSCode).

The nice thing about the xCode project is that it also builds for both platforms, outputting a fat (universal) binary as does VSCode (via CMake). However, where the CMake method seems to sometimes forget about building the "other" platform, xCode is reliable in this way. :-) But I have not tested building Release builds on xCode, so there may be differences or problems that could arise if you try to use the Release build made from this method. (In other words, no promises. I recommend using VSCode + CMake (#1) to build for macOS.


---
#### __Option 4: MS Visual Studio C++ to build locally (Win)__:
---
This seems to be supported, but honeslty I (Steve) have never looked at this.  So far my focus
has been on the Mac version.  Others are welcome to improve this, though it'd be ideal if we
can instead make the Docker version work better, i.e. streamline it to strip symbols and
reduce the total size which is currently exhorbitant (see above) and has not been verified.

---

## Preparing a release:

There is no script available yet to build a full release. However, the steps to building a release are below.  Assuming you are able to build natively on a "modern" macOS machine (i.e., running macOS  10.13 or later), and you can run VSCode, CMake, and Docker on the same machine:

1. Open the VSCode project for your local blu_fx source tree (wherever it is).  For example, for the first time, maybe you cloned this project or forked your own version and want to build to start out:
   ```
   % cd ~/Documents/src/
   % git clone https://github.com/path/to/blu_fx_repo blu_fx
   % cd blu_fx
   % code .   # assuming VSCode command line tools installed for "code" command
   ```
3. In VSCode, either manually or via the extension support if you've installed the CMake extensions needed, make the Release build.
4. Double-check you have both platlforms built (assuming it succeeded), using the embedded shell (usually a pane at the bottom):
   ```
   % lipo -archs ./build/blu_fx.xpl
   x86_64 arm64
   ```
   If the result is not "`x86_64 arm64`", then shake that tree! (Try building again.)
5. In the same embedded shell as above, build for Linux and Windows on Docker (assuming you have it installed and configured) .. it should install everything it needs inside Docker, you just need to have it available. It will cache everything after this, so it only takes forever the very first time:
   ```
   % (cd docker ; make lin win)    # no "mac" for now since we have to make it separately (above)
   ```
6. Assuming you've tested what you can locally, and are sure things are okay, now you need to **notarize** the macOS build, which is not documented here (yet) nor possible without external resources (until I am able to add a script here to help automate it for you). So, without specifying _how_, please use this step to ensure the file "`build/blu_fx.xpl`" is both signed and notarized according to Apple's policies for your third-party developer code to be able to run freely in X-Plane on any macOS machine.
7. Put it all together in a new release folder. I recommend taking the latest release you've found online, downloading it into a folder called "`~/Downloads/blu_fx`", and simply adding the updated files in-place there, as follows:
   ```
   % mkdir -f ~/Downloads/blu_fx/{mac,win,lin}_x64    # not needed if already there
   % cp -a build/blu_fx.xpl ~/Downloads/blu_fx/mac_x64/      # (signed/notarized) mac binary
   % cp -a build-lin/blu_fx.xpl ~/Downloads/blu_fx/lin_x64/  # Linux binary
   % cp -a build-win/blu_fx.xpl ~/Downloads/blu_fx/win_x64/  # Windows binary
   % date ; touch ~/Downloads/blu_fx                  # just to set modification time of main folder
   (will show the current date and time)
   % vi ~/Downloads/blu_fx/README.txt                 # or however you want, i.e. to update release notes
   % vi ~/Downloads/blu_fx/Version.txt
     (edit the version number as needed, and copy the date from above and paste it over the old one)
     It should look like:
     "blu-fx_v1.2  (packaged on Wed Jul 31 13:58:32 PDT 2024)"
   ```
8. Go to Finder, navigate to the Downloads folder and right-click on the folder "`blu_fx`", and choose "`Compress "blu_fx"`" from the context menu to create the .zip file.
9. Rename the newly-created .zip file to be called something useful, but distinguishable from the contained folder (!!) so users won't install the wrong thing if they unzip and have a folder with the same name as the container.  So, I recommend changing the name of the .zip file to be something like:
   ```
   blu_fx_v1.4.2b1.zip
   ```
11. Upload to wherever you're going to let people download it from, or send it to whoever you want to send it to, or just unzip it again to a separate place, in order to try it locally to verify everything worked end to end. Enjoy. :-)
