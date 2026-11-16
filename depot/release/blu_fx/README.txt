BLU-fx v1.2.3                                         Released: 24-January-2026
-------------------------------------------------------------------------------
This is BLU-fx redux, v1.2.3, by Steve Goldberg (from the original by Matteo
Hausner). This version is now hosted at <https://github.com/slgoldberg/BLU-fx>.

** NOTE: you should NOT download the binary from GITHUB. Instead, please use
         the release available from the Org Forum, and/or you can easily
         update this via Skunkcrafts Updater Standalone.

BLU-fx as of v1.2 is supported on Skunkcrafts Updater Standalone!  Make sure
you copy the ".cfg" files from this folder in the .../Resources/plugins/blu_fx
folder in X-Plane.  Then, when you run Skunkcrafts Updater Standalone, it will
discover the plugin and always make it trivial for you to update to the last
release -- whether as a "beta" or "release" build.

---

Built on 24-January-2026 as a 64-bit plugin for X-Plane 11 and 12,
on Windows, Mac (Apple Silicon or Intel), and Linux. This plugin is free
(see more information below) and ONLY runs on "modern" versions of X-Plane,
starting with X-Plane 11.10 and later -- including the latest X-Plane 12.

IMPORTANT: This version of BLU-fx will NOT run on older versions of X-Plane
           prior to v11.10!  Any users wishing to use BLU-fx on those versions
           should continue to use the original v1.0 release by Matteo Hausner.
           (Note: Since X-Plane 11.10 and up *only* accept 64-bit binary
           format for plugins, there is also no way to use this version on a
           system that doesn't support 64-bit binaries.)

---

RELEASE NOTES for BLU-fx v1.2.3 (released 24-January-2026):

This is a minor release, with a focus on just one main change (below):

v1.2.3: (January, 2026)

  * Fix minor issues that might cause complaints in X-Plane v12.4.0, due
    to checks that X-Plane now does to ensure that plugins don't access
    XPLM features before their XPluginStart() function is called.  (We
    were only doing one little thing we shouldn't have been, though it
    did NOT set off any alerts or asserts in X-Plane, we're being safe
    by moving that one thing to run-time.)  A side-effect of this is
    that we also intentionally ignore any attempts to modify the
    X-Plane-11-only feature "Cinema Verite" setting in newer XP12 versions.

v1.2.2:

  * Change the order of startup and shutdown of all OpenGL functionality
    based on the requirements for plugin uses there in environments with
    "modern graphics drivers" (which means, X-Plane v11.50+ in Vulkan/Metal
    mode, or any version of X-Plane v12 and later).

    - This is only done as a precaution, to eliminate the possibility of
      timing issues that *could* have been present with the very early
      initialization (and late tear-down) of the OpenGL library calls from
      within BLU-fx prior to this version v1.2.2.

    - The best case scenario is that this fixes a potential random CTD that
      only one user has so far ever reported, and in the best case, it makes
      it so users can choose to disable and re-enable BLU-fx dynamically from
      within the Plugin Manager UI in X-Plane, which will actually entirely
      remove the plugin's functionality without having to reload plugins or
      restart the simulator.

---

Previous releases:

v1.2.1:

  * When the current configuration matches one of the "presets" displayed
    in the button array in the Settings window, disable that preset button
    so that the user can tell that that preset is already loaded.

    - This was needed because, for example, when taking screen shots to add
      to the main BLU-fx download page, it was impossible for Yours Truly to
      keep track of which screen shot corresponded to which preset!  Now, the
      selected preset will be rendered differently, since it's "disabled",
      making it trivial to tell which preset is loaded for any arbitrary
      screen shot.

    - Of course, this also has the benefit of helping users recognize when
      a preset that's been saved and restored as the current configuration
      matches a preset configuration, since otherwise, there's no real way
      for the user to tell.

---

PRIOR RELEASE NOTES from v1.0 to v1.2:

This is generally the same user interface as the original release, however it
includes support for X-Plane 11 and X-Plane 12 (to the exclusion of any older
versions!), and introduces a way to readily restore the current filter
configuration to the one loaded from the .prf file.  It also allows users to
swap the .ini file behind the scenes with a different one, and to hit a button
in the settings window to force-load the .ini file again, so you no longer need
to quit and restart X-Plane just to try out a different saved configuration.

* Change the Settings window in the following ways:
  - Hide the "Raleigh" slider in X-Plane 12, since Raleigh does not exist (in
    the same way), and so any saved Raleigh value will be ignored in XP12.
  - Fix the window and associated fonts to be sized and rendered correctly
    when users choose any "UI Zoom" in X-Plane that is not 100%.  In other
    words, switch to using "modern" windows in X-Plane, to have the plugin
    correctly honor the UI scaling requested by users via X-Plane "UI Zoom".  
  - Add a button to reload the current filter state (i.e, the values of all
    the slliders and options) from the .ini file on-demand, instead of only
    loading it at startup.
  - Add a button to write the current filter state back into the .ini file,
    replacing the contents of whatever was there before. This always happened
    automatically at simulator shutdown time. This means in addition to always
    writing at shutdown, BLU-fx now also writes the .ini file when you ask
    it to!
  - Fix Blu-FX to actually work on all modern platforms, from X-Plane 11.10
    through X-Plane 12 (latest release as of this writing.) This comes,
    however, at the cost of no longer supporting older versions of X-Plane,
    including 32-bit versions of the operating system.
  - Other improvements to layout, functionality, and features. However,
    some of the older features have not been fully tested and/or are
    disabled, so do not expect EVERYTHING to work as before. The main
    function, however -- providing free "reShade-like" functionality for XP11
    and XP12 -- definitely works great, even on the very latest XP12 release!

This IS backwards compatible with your previous configuration files! We hope
to make it easier to manage various configurations in the future, too.

Support will be coming "soon" for Skunkcrafts Updater Standalone. Please
check back within a few weeks of when you first downloaded this. If the
new ZIP file includings files with names like "skunkcrafts_updater.cfg", then
copy those files into your installed ABC folder, and it will update like magic.

----

For previous release notes or other information, please see the archived
README and other information associated with the original open-source
repository for this plugin at <https://github.com/bwRavencl/BLU-fx>.*

* The original repository has been archived, since Matteo is no longer able
to support that version. Any requests should be directed to Steve Goldberg
(via private message on the X-Plane.org forums to @slgoldberg), referencing
the forked repository where all newer development is located. (The new
repository is at <https://github.com/slgoldberg/BLU-fx>.)

For any questions or comments, please contact Steve (information above).
