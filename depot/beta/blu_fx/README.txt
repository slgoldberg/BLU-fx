BLU-fx -- Real-Time Color Grading for X-Plane 12 & 11
Copyright (C) 2026 by Steven L. Goldberg (@slgoldberg)
v2.1.0b1 (packaged on Sat Nov 28 05:47:08 UTC 2026)
See ReleaseNotes.txt for full change history.
===============================================================================

BLU-fx is a lightweight, real-time post-processing color grading plugin for
X-Plane 12 and X-Plane 11 across macOS, Windows, and Linux. It allows you to
customize scene brightness, contrast, color temperature, and atmospheric styling
with zero noticeable impact on simulator framerates.

Whether running under OpenGL, Vulkan, or Metal, BLU-fx delivers instantaneous,
customizable visual post-processing directly from an intuitive, in-cockpit
settings window.


DOCUMENTATION & USER GUIDE
-------------------------------------------------------------------------------
BLU-fx includes comprehensive, illustrated user documentation:

  * In-Simulator Viewer: Open X-Plane and select Plugins > BLU-fx > User Guide,
    or click the User Guide button in the Settings window header.
  * Bundled PDF Manual: A standalone, publication-quality manual is provided in
    the package at:
      docs/BLUFX_User_Guide.pdf
  * Detailed Release Notes: For a full list of what is new in this release and
    historical changes, see:
      ReleaseNotes.txt


INSTALLATION
-------------------------------------------------------------------------------
1. Extract the downloaded ZIP archive.
2. Copy or move the entire "blu_fx" folder into your X-Plane plugins directory:
     <X-Plane>/Resources/plugins/blu_fx/
3. Verify that the architecture subdirectories are present:
     .../Resources/plugins/blu_fx/mac_x64/   (macOS Apple Silicon & Intel)
     .../Resources/plugins/blu_fx/win_x64/   (Windows 64-bit)
     .../Resources/plugins/blu_fx/lin_x64/   (Linux 64-bit)
4. Start X-Plane. Open the simulator menu and choose:
     Plugins > BLU-fx > Settings window


SKUNKCRAFTS UPDATER SUPPORT
-------------------------------------------------------------------------------
BLU-fx provides full native support for the SkunkCrafts Updater standalone
desktop application across release, beta, and daily channels.

  * Automatic Discovery: Keep the bundled "skunkcrafts_updater.cfg" file inside
    your blu_fx directory. When you run SkunkCrafts Updater, it will automatically
    detect your installation and notify you when new builds are available.
  * Channel Selection: You can easily switch between release, beta, and daily
    channels from the SkunkCrafts Updater desktop client.
  * If you do not have SkunkCrafts Updater, you can download it for free from
    the X-Plane.org forum.


DOWNLOADS & COMMUNITY SUPPORT
-------------------------------------------------------------------------------
  * Official Download Page:
      https://forums.x-plane.org/index.php?/files/file/40960-blu-fx/
  * GitHub Source Repository:
      https://github.com/slgoldberg/BLU-fx
  * Support & Feedback:
      Contact Steve Goldberg via private message on the X-Plane.org forums
      (@slgoldberg) or report issues on GitHub.


CREDITS & LICENSE
-------------------------------------------------------------------------------
Original version (v1.0) created by Matteo Hausner (bwRavencl).
Modern revitalizations, X-Plane 12 modernization, ImGui architecture, and
all subsequent releases (v1.1 through v2.0+) developed and maintained by
Steve Goldberg (@slgoldberg).

Distributed under the terms of the GNU General Public License v2 (GPL-2.0).
See gpl-2.0.txt for full details.
