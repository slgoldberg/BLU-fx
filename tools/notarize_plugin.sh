#!/bin/bash
#
# notarize_plugin.sh - A shell script to automate signing and notarizing X-Plane plugins with
#                      a certified Apple Developer ID.
#
# Originally written by Chris at Laminar Research, Inc., and posted for general use on the
# X-Plane.org forum.
#
# Modified by Steven L. Goldberg (@slgoldberg on the X-Plane.org forums, brat@footbag.org to rework
# the script to use the new and improved xCode command-line tools for this. (For example, with the
# "right" settings, you no longer need to wait for a response; the command will do it for you.)
#
# ------
# 
# USAGE: ./notarize_plugin.sh <path_to_dylib.xpl>
#
#   <path_to_dylib.xpl> This is the path to your plugin is a full (or relative) file
#                       path to the .xpl file on disk, where it is unzipped and ready
#                       to run on the local machine.
#                       (Note: Don't point to a .zip file, but to the actual .xpl file
#                       you want notarized. After notarization, you can then COPY or
#                       MOVE this file to the final installation location, which you
#                       can then ZIP up and distribute however you like.)
#
# Example:
#     From the home directory of the Blu-FX tree on the local machine, type:
#     % ./tools/notarize_plugin.sh ./build/blu_fx.xpl
#
# Note: For anyone who used the "old" Laminar notarization script, you'll note this
# is much simpler, in part because notarization of a dylib doesn't have any need for
# nor a concept of a bundle ID, so that's not a parameter anymore. Also, the script
# doesn't need to be as complicated with all the looping waiting for a response, since
# the command-line tool now has a built-in "--wait" option. :-)
#
# ---
#
# To use this script, you must have a valid certificate with public/private key pair for
# app signing for your copmany (or as an individual) that is NOT EXPIRED (and from time to
# time you may need to accept new Terms & Conditions at developer.apple.com if they change
# and require acceptance before you can use them for notarization. (You must also be sure
# to use Apple's developer tools aka xCode Developer SDK from versions starting with 10.14
# or later. (Update: I think this script now assumes a later versions; so try 10.15 or
# maybe even 10.16 if the script fails due to an unknown parameter, for example.)
#
#------- CODE SIGNING AND NOTARIZATION UTILS FOR MAC --------

die() {
	echo $1
	exit 1
}

# ----------------------------------------------------------------------------
# CONFIGURATION - YOU NEED TO FILL THIS STUFF IN at least once for your world!
#
# Parameters you'll need to modify at least once:
#
#     developer_id=
#         This should be the name of your app distribution certifiate, e.g.,
#         'developer_id="Developer ID Application: Acme Co"'.
developer_id="<replace_me>"
#
#     app_password=
#         This should be the name of the login keychain password item that
#         contains the "app password" you got from Apple.  (The name here
#         doesn't have to be codesigning, it's the item name you pick when you
#         stash the password.)
#app_password=@keychain:AC_PASSWORD           # note: this doesn't work for me
# Instead, I use the actual string app password in the clear for now:
# (Note: of course this won't work for any other uses than specifically mine.)
app_password=<replace_me>
#
#     username=
#         This is your Apple ID you log in with, e.g. it might be your email
#         address or maybe it's allowed to be a phone number (I'm not positive):
username=<replace_me>
#
#     team_id=
# This is your team ID per Apple developer site needed for notarytool (and this
# also confuses me and I'm not sure it's needed anymore as of the newer SDK):
team_id=<replace_me>
#
# END CONFIGURATION. (Set each parameter above before you try this script.)
# ----------------------------------------------------------------------------

# notarize </path/to/mac_x64/plugin.xpl>
code_sign() {
	echo "Code signing $1"
	local plg_path=$1
	local tool_results="$plg_path".txt
	local plg_path_zip="$plg_path".zip
	rm -rf "$plg_path_zip"
	codesign --force --timestamp --options=runtime --deep \
		-s "$developer_id" \
		"$plg_path"
	zip -rq --symlink "$plg_path_zip" "$plg_path"
	echo xcrun notarytool submit --wait --apple-id $username --team-id $team_id --password ___ "$plg_path_zip"
	echo "(Waiting for notarization...)"
	xcrun notarytool submit --wait --apple-id $username --team-id $team_id --password $app_password "$plg_path_zip"
	rm -rf "$plg_path_zip"
}

#---------------------- MAIN SCRIPT --------------------------

if [ $# -ne 1 ]; then
	die "Usage: $0 </path/to/mac_x64/plugin.xml>"
fi

plugin_path=$1

# notarize binaries
code_sign "$plugin_path"

# Now you just have to wait for it to finish. :-) Notice that "notarytool" is passed
# the parameter "--wait", which causes it to wait until there's an answer. If it
# doesn't show an error, then that means it's approved. You can copy the .xpl file
# you pointed to in the command-line parameter, anywhere you like, and ZIP up however
# you like, as many different times and ways as you like. :-)
