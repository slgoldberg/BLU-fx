#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# I indent with tabs (size 4) and this is not up for discussion. If unhappy with it, get a life.
# License: come on, these are only a few lines of code
# Do whatever you want with them, at your own risks, except saying you wrote them.
# v3

# begin of user config
# always use / (no backslash), even on windows.
# ftp servers must support MLSD command (RFC 3659)
zones = {
	"release": {
		"local" : "/Users/brat/Documents/blufx2/skunkcrafts/release",
		"remote": "/Volumes/footbag/media/skunkcrafts/depot/release"
	},
	"beta": {
		"local" : "/Users/brat/Documents/blufx2/skunkcrafts/beta",
		"remote": "/Volumes/footbag/media/skunkcrafts/depot/beta"
	},
	"daily": {
		"local" : "/Users/brat/Documents/blufx2/skunkcrafts/daily",
		"remote": "/Volumes/footbag/media/skunkcrafts/depot/daily"
	},
}
# end of user config

import os , sys, glob, io, time
import binascii
import shutil
import ftplib
import tempfile

commands	= ["lock", "unlock", "status", "check", "update", "deadmeat", "init"]
notdeadmeat	= ["skunkcrafts_updater_sizeslist.txt", "skunkcrafts_updater_whitelist.txt", "skunkcrafts_updater_blacklist.txt", "skunkcrafts_updater.cfg"]
ignorelist	= ["skunkcrafts_updater_sizeslist.txt", "skunkcrafts_updater_whitelist.txt", "skunkcrafts_updater_blacklist.txt", "skunkcrafts_updater_oncelist.txt", "skunkcrafts_updater_ignorelist.txt", "skunkcrafts_updater_config.txt"]
blacklist	= []
oncelist	= []
deltalist	= {}
newwhite	= {}
oldwhite	= {}
sizeslist	= {}

rmtconfig	= "skunkcrafts_updater.cfg"
lclconfig	= "skunkcrafts_updater_config.txt"
blackname	= "skunkcrafts_updater_blacklist.txt"
whitename	= "skunkcrafts_updater_whitelist.txt"
sizesname	= "skunkcrafts_updater_sizeslist.txt"
ignorename	= "skunkcrafts_updater_ignorelist.txt"
oncename	= "skunkcrafts_updater_oncelist.txt"

def bozo(reason):
	print(reason)
	print(f"Usage: {sys.argv[0]} zone aircraft command")
	print("Commands: lock, unlock, check, update, deadmeat")
	exit(1)

def fatal(reason):
	print(f"Fatal error: {reason}")
	exit(1)

def success(reason):
	print(f"Success: {reason}")
	exit()

def CRC32_from_file(filename):
	buf = open(filename,'rb').read()
	buf = (binascii.crc32(buf) & 0xFFFFFFFF)
	return f"{buf:d}"

def rmtconfig_read():
	print("Reading remote config")
	config	= {}
	def readcfg(line):
		fi = line.rstrip().split("|")
		if len(fi) == 2:
			config[fi[0]] = fi[1]

	if "remote" in zone:
		with open(f"{rem}/{rmtconfig}", "r") as f:
			for ligne in f:
				readcfg(ligne)
			return(config)
	else:
		try:
			ftp.cwd(ftp_base)
			ftp.retrlines(f"RETR {rmtconfig}", readcfg)
			return config
		except ftplib.all_errors as error:
			fatal(f"{ftp_fqdn}: RETR {ftp_base}/{rmtconfig} failed: {error} ")

def rmtconfig_write(config):
	print("Writing remote config")
	buf = io.BytesIO()
	for k, v in config.items():
		buf.write(str.encode(f"{k}|{v}\n"))
	buf.seek(0)

	if "remote" in zone:
		with open(f"{rem}/{rmtconfig}", "wb") as f:
			# Écrit le buffer dans le fichier
			f.write(buf)
	else:
		try:
			ftp.cwd(ftp_base)
			ftp.storbinary(f"STOR {rmtconfig}", buf)
		except ftplib.all_errors as error:
			fatal(f"{ftp_fqdn}: STOR {ftp_base}/{rmtconfig} failed: {error} ")

def change_lock(lock):
	print("Changing lock")
	config	= rmtconfig_read()
	lock	= "true" if lock else "false"
	if "locked" in config and config["locked"] == lock:
		print(f"Lock already {lock}")
	config["locked"] = lock
	rmtconfig_write(config)

def get_lock():
	config	= rmtconfig_read()
	return "locked" in config and config["locked"] == "true"

def load_list(name, lst):
	fname	= f"{src}/{name}"
	try:
		with open(fname, "r") as fh:
			lst += fh.read().splitlines()
		for line in lst:
			line = line.replace("\\","/")
	except IOError as error:
		fatal(f"failed to read {fname} : {error}")

def ftp_check_dir(mydir):
	try:
		ftp.cwd(mydir)
		return
	except ftplib.all_errors as error:
		ftp.mkd(mydir)
		ftp.cwd(mydir)

def ftp_chdir(ftp_path, ):
	dirs = [d for d in ftp_path.split('/') if d != '']
	for p in dirs:
		print(p)
		check_dir(p)

def check_dir(folder):
	filelist = []
	ftp.retrlines('LIST', filelist.append)
	found = False

	for f in filelist:
		if f.split()[-1] == folder and f.lower().startswith('d'):
			found = True

	if not found:
		ftp.mkd(folder)
	ftp.cwd(folder)

# ~ def ftp_check_dir(mydir):
	# ~ for entry, facts in ftp.mlsd():
		# ~ if entry == mydir and facts["type"] == "dir":
			# ~ ftp.cwd(entry)
			# ~ return
	# ~ ftp.mkd(mydir)
	# ~ print(f"cwd({mydir})")
	# ~ ftp.cwd(mydir)

# ~ old_ftp_path = "something that does not exists"
# ~ def ftp_chdir(ftp_path):
	# ~ print(f"chdir to: {ftp_path}")
	# ~ global old_ftp_path
	# ~ if old_ftp_path == ftp_path:
		# ~ return
	# ~ ftp.cwd(f"{ftp_base}")
	# ~ dirs = [d for d in ftp_path.split('/') if d != '']
	# ~ for p in dirs:
		# ~ ftp_check_dir(p)
	# ~ old_ftp_path = ftp_path

def ftp_chdir(ftp_path):
	try:
		# ~ print(f"trying ftp.cwd({ftp_path})")
		ftp.cwd(ftp_path)
		return
	except ftplib.all_errors as error:
		ftp_less, folder = os.path.split(ftp_path)
		# ~ print(f"Error, now trying {ftp_less}")
		ftp_chdir(ftp_less)
		wd = ftp.pwd()
		# ~ print(f"In {wd} doing ftp.mkd({folder})")
		ftp.mkd(folder)
		# ~ print(f"Retrying chdir to {ftp_less}")
		ftp_chdir(ftp_path)

def ftp_send(src, dst):
	try:
		fh = open(src, "rb")
	except IOError as error:
		fatal(f"failed to open {src}, partial update! : {error}")

	try:
		path, dest = os.path.split(f"{ftp_base}/{dst}")
		ftp_chdir(path)
		# ~ wd = ftp.pwd()
		# ~ print(f"ftp.pwd() = {wd}")
		# ~ print(f"STOR {dest}")
		ftp.storbinary(f"STOR {dest}", fh)
		fh.close()
	except ftplib.all_errors as error:
		fatal(f"{ftp_fqdn}: ftp_chdir({path}) then STOR {dest} failed: {error} ")

def rem_send(src, dst):
	path, dest = os.path.split(f"{src}")
	if not os.path.exists(f"{rem}/{path}"):
		os.makedirs(f"{rem}/{path}")
	#print(f"src:{src} -> dst:{rem}/{dst}")
	shutil.copy(src, f"{rem}/{dst}")

def tree_ftp(mylist, mypath):
	for entry, facts in ftp.mlsd():
		f = facts["type"]
		if f == "dir":
			pwd = ftp.pwd()
			ftp.cwd(entry)
			tree_ftp(mylist, mypath + "/" + entry)
			ftp.cwd(pwd)
		elif f == "file":
			mylist.append(str.lstrip(mypath + "/" + entry, "/"))

def tree_local(mylist, mypath):
	#print(f"tree_local {mylist}, {mypath}")
	try:
		for entry in os.listdir(rem + mypath):
			f = os.path.isdir(rem + mypath + "/" + entry)
			if f:
				tree_local(mylist, mypath + "/" + entry)
			else:
				mylist.append(str.lstrip(rem + mypath + "/" + entry, rem))
	except:
		mylist.append(str.lstrip(rem + mypath + "/" + entry, rem))

def read_oldwhite(line):
	fi = line.rstrip().split("|")
	if len(fi) == 2:
		oldwhite[fi[0].replace("\\","/")] = fi[1]

####
####
####

if len(sys.argv) != 4:
	bozo(f"Incorrect number of arguments {len(sys.argv)}")

zone	= sys.argv[1]
acf		= sys.argv[2]
cmd		= sys.argv[3]

src = "/tmp"
dst = "/tmp"

if not zone in zones:
	fatal(f"Unknown zone {zone}")

if cmd not in commands:
	bozo(f"Command '{cmd}' not recognized")

name = zone
zone = zones[zone]
src  = (zone["local"] + "/" + acf).replace("//", "/")
if not os.path.isdir(src):
	fatal(f"{src} is not a valid folder")

if cmd == "init":
	print(f"Creating {acf} in zone {name}")
	if "remote" in zone:
		rem	= (zone["remote"] + "/" + acf).replace("//", "/")
		if not os.path.isdir(rem):
			os.makedirs(rem)
		with open(f"{rem}/{whitename}", "wb") as f:
			print(f"Creating {rem}/{whitename}")

	else:
		ftp_fqdn	= zone["ftp"]
		ftp_user	= zone["user"]
		ftp_pass	= zone["pass"]
		ftp_base	= (zone["base"]).replace("//", "/")
		try:
			print(f"ftp to: {ftp_fqdn}")
			ftp = ftplib.FTP_TLS(ftp_fqdn)
			print(f"login with: {ftp_user}")
			ftp.auth()
			ftp.login(user=ftp_user, passwd=ftp_pass)
			print(f"cwd to: {ftp_base}")
			ftp.cwd(ftp_base)
			ftp_check_dir(acf)
			with tempfile.NamedTemporaryFile() as temp:
				ftp.storbinary(f"STOR {whitename}", temp)
		except ftplib.all_errors as error:
			fatal(f"{ftp_fqdn}: STOR {ftp_base}/{acf}/{whitename} failed: {error} ")
	success("Remote depot initialized")

if "remote" in zone:
	rem	= (zone["remote"] + "/" + acf).replace("//", "/")
	print(f"Remote is {rem}")
	if not os.path.isdir(rem):
		fatal(f"{rem} is not a valid folder")
else:
	ftp_fqdn	= zone["ftp"]
	ftp_user	= zone["user"]
	ftp_pass	= zone["pass"]
	ftp_base	= (zone["base"] + "/" + acf).replace("//", "/")

	try:
		print(f"ftp to: {ftp_fqdn}")
		ftp = ftplib.FTP_TLS(ftp_fqdn)
		print(f"login with: {ftp_user}")
		ftp.auth()
		ftp.login(user=ftp_user, passwd=ftp_pass)
		print(f"cwd to: {ftp_base}")
		ftp.cwd(ftp_base)
	except ftplib.all_errors as error:
		fatal(f"ftp to {ftp_fqdn} as {ftp_user} failed: {error} ")





if cmd == "lock":
	change_lock(True)
	success("Remote depot locked")

if cmd == "unlock":
	change_lock(False)
	success("Remote depot unlocked")

if cmd == "status":
	success(f"Remote depot lock is {get_lock()}")

os.chdir(src)


# Get remote whitelist
print("Getting remote whitelist")

if "remote" in zone:
	with open(f"{rem}/{whitename}", "r") as f:
		for ligne in f:
			read_oldwhite(ligne)
else:
	try:
		ftp.retrlines(f"RETR {whitename}", read_oldwhite)
	except ftplib.all_errors as error:
		fatal(f"{ftp_fqdn}: RETR {ftp_base}/{whitename} failed: {error} ")

if cmd == "deadmeat":
	mylist = []
	print("Files in depot but not in whitelist:")

	if "remote" in zone:
		for file in glob.iglob(f"{rem}/**/*.*", recursive=True):
			mylist.append(file.replace(f"{rem}/", ""))
			#print(file, file.replace(f"{rem}/", ""))
		#mylist = glob.glob(f"{rem}/**/*.*", recursive=True)
		#print(files)

	else:
		ftp.cwd(ftp_base)
		tree_ftp(mylist, "")

	for entry in sorted(mylist, key=str.lower):
		if entry not in oldwhite and entry not in notdeadmeat:
			print(entry)
	exit()

# Populate lists
print("Getting ignorelist")
load_list(ignorename, ignorelist)
print("Getting blacklist")
load_list(blackname	, blacklist)
print("Getting oncelist")
load_list(oncename	, oncelist)

# Build local whitelist
print("Populating new white list")
for entry in sorted(glob.iglob("**", recursive = True), key=str.lower):
	if os.path.isfile(entry):
		posix_entry = entry.replace("\\","/")
		if posix_entry in oncelist:
			newwhite[posix_entry] = -1
			sizeslist[posix_entry] = os.path.getsize(entry)
		elif posix_entry in ignorelist:
			pass
		else:
			newwhite[posix_entry] = CRC32_from_file(posix_entry)
			sizeslist[posix_entry] = os.path.getsize(entry)

with open(whitename, "w", newline='\n') as fh:
	for entry, crc in newwhite.items():
		fh.write(f"{entry}|{crc}\n")

with open(sizesname, "w", newline='\n') as fh:
	for entry, crc in sizeslist.items():
		fh.write(f"{entry}|{crc}\n")

# Look for delta
print("Looking for new/modified files")
for entry, crc in newwhite.items():
	posix_entry = entry.replace("\\","/")
	if posix_entry in oldwhite:
		if crc != oldwhite[posix_entry] and crc != -1:
			deltalist[posix_entry] = f"new crc {crc} (old was {oldwhite[posix_entry]}"
	else:
		deltalist[posix_entry] = "new file"

if cmd == "check":
	for entry, reason in deltalist.items():
		print(f"Need to send '{entry}' : {reason}")
	if len(entry) == 0:
		print("Nothing to do")
	exit()

if cmd == "update":
	if "remote" in zone:
		for entry, reason in deltalist.items():
			print(f"Sending '{entry}' ({reason})")
			rem_send(entry, entry)
		print(f"Sending {whitename}")
		rem_send(whitename, whitename)
		print(f"Sending {blackname}")
		rem_send(blackname, blackname)
		print(f"Sending {sizesname}")
		rem_send(sizesname, sizesname)
		print(f"Sending {rmtconfig}")
		rem_send(lclconfig, rmtconfig)
		success(f"Update finished and remote depot lock is {get_lock()}")

	else:
		for entry, reason in deltalist.items():
			print(f"Sending '{entry}' ({reason})")
			ftp_send(entry, entry)
		print(f"Sending {whitename}")
		ftp_send(whitename, whitename)
		print(f"Sending {blackname}")
		ftp_send(blackname, blackname)
		print(f"Sending {sizesname}")
		ftp_send(sizesname, sizesname)
		print(f"Sending {rmtconfig}")
		ftp_send(lclconfig, rmtconfig)
		success(f"Update finished and remote depot lock is {get_lock()}")

	exit()

print("Control should not reach this statement")
exit()
