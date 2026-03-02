
	Welcome to Tinfoil Hat Linux (THL)
   An exercise in paranoia or a day to day tool. 

The purpose of this distribution is to have a safe 
location to encrypt and sign documents using Gnu Privacy Guard. 

You may want to Tinfoil Hat Linux when ....
*****************************************************
   * You're using a computer that could have a keystroke logger installed. 
www.keyghost.com is an example of a tiny & cheap hardware loggers. 
There are smaller & cheaper ones out there, and they're all hard to notice. 
Look for them the next time you're using a public terminal at a convention 
or hosting facility. 
   * Using your personal GPG keys at work, school or a web cafe where
 you don't trust or own the equipment. 
   * If you maintain a PGP Certificate Authority and have to have a safe
 place to use the CA key. 
   * If you're with witness.org &/or have to hide the very fact 
 that you have PGP. 
   * If you simply don't want to risk putting a PGP key on a hard drive
 where someone else might have access to it. 
   * The Illuminati are watching your computer, and you need to use morse code
 to blink out your PGP messages on the numlock key. 

What Tinfoil Hat Linux protects against ....
***************************************************
* Worms & viruses
The OS doesn't support networking, all binaries are compiled staticly, 
and all non-root partitions are mounted with no-execute permissions. 
A hash of the NVRAM is displayed at boot time. 

* Data retrieval
All temporary files are created on an encrypted ramdisk which is destroyed on
shutdown. Even the PGP keyfile information can be stored encrypted on the 
floppy.   

* Keystroke monitoring. 
THL has gpggrid , a wrapper for GPG that lets you use a video game style 
character entry system instead of typing in your passphrase. Keystroke 
loggers get a random set of grid points, not your passphrase. 

* Power usage & other side channels. 
If you start the Paranoid options,  a copy of GPG runs in the background
generating keys & encrypting random documents. This makes it harder to 
determine When your REAL encryption is taking place. See the TEMPEST section
below.

* (some) User stupidity. 
If you use THL, it's very difficult to leave a plaintext file on your
hard drive by accident. 


Setting up Tinfoil Hat Linux ..
***********************************************
You must first copy the Tinfoil hat disk image to a floppy disk. 
If you don't know how to do this, google search for "rawrite" or
"dd of=/dev/fd0". The command you will use is  "rawrite thl.img a: " 
or "dd if=thl.img of=/dev/fd0"

* If you already have gpg:
1a) Copy the contents of your $HOME/.gnupg directory into the
 gnupg directory on the floppy.  or
1b) If you use unix & don't want to leave your GPG information unencrypted on 
a floppy, run the following command:  
"tar -cvf - $HOME/.gnupg |gpg -co /path/to/floppy/ring.gpg" 
This will create a symmetrically encrypted backup of your keyring. 
2) Run gpg with the following command :
gpg --gen-random 2 512 > /path/to/floppydisk/entropy.bin
This will create a file called entropy.bin on the floppy that will be used
to make sure GPG's encryption is not predictable. 


How to use Tinfoil Hat Linux  ....
***************************************************
If at all possible, boot THL on a laptop & disconnect all external
   cables, including the power & mouse.  Turn off nearby 
   radios, including cell phones and microwaves.  Put yourself 
   and the computer in a well grounded opaque copper cube.  Download
   your tinfoil hat plans from http://zapatopi.net/afdb.html. 
   Boot the floppy....

1) After a while you will be presented with a fairly friendly menu.
You can continue to use the functions on the menu, or type x to get
a shell. 
2) Load encrypted messages from the floppy, edit them on THL, and then
save new encrypted messages back to the floppy. 
3) When you are finished, make sure to run the "shutdown" command,
or select shutdown from the menu. This command backs up your keyring
from RAM to the floppy. 
4) Reboot into your normal OS, pop in the floppy with your encrypted 
messages, and email people the encrypted files from the floppy. 

Stuff to worry about...
**********************************************
* Tempest & EMSEC
** CRT: THL uses ctheme to manipulates the VGA console palette. It's an 
amusing hack, and does make it harder to photograph the screen with a
digital camera, but it won't complicate tempest observation.  It's the 
best I could figure out without having greyscale fonts. 

In order to complicate listening to radiation from the keyboard, THL blinks
encrypted messages in morse code on the keyboard LEDs. (you can also use these 
programs to display messages without the CRT. ) (I'm not sure if the
keyboard is actually returning answers to a query, or just receiving commands
from the CPU. Need to add in explicit queries, so more traffic is coming 
from the keyboard)

** USB: USB support increases the entropy, and it's currently harder to 
put a sniffer on a USB keyboard. But USB cables are usually long & not well
 shielded which makes them good antennas. 

* Entropy. 
The real problem with a embedded system like this is coming up with 
enough entropy. Using a noisy diode/geiger counter on the serial port
would be nice.  

* The sources used to build this software. 
There aren't  patches to anything but busybox, and the only risky change 
is using uClibc to compile gpg.  No code has been audited, but all signed
files were verified. 

gpg 1.0.6 was compiled using uClibc and the following options:
CC="gcc -static" ./configure --disable-ldap --disable-mailto --disable-largefile --disable-nls  --with-included-zlib --with-included-gettext --disable-dynload --enable-static-rnd=linux  

Busybox is patched to add "random" with the patch from http://tinfoilhat.cultists.net/source/bb-random.gz . Busybox was then compiled with uclibc. 
gpggrid source is at http://tinfoilhat.cultists.net/source

GNU nano is compiled staticly with uclib and a static ncurses (also compiled with uclibc) 
losetup & mount are from unix-linux-2.11i.tgz,patched with loop-AES-v1.5b.tar.bz2 (loop-aes.sourceforge.net)
blinker and text2morse are from morse2led-1.0.tgz (compiled against uclibc)
wipe is from wipe.sourceforge.net
ctheme is from http://sourceforge.net/projects/ctheme

*******************TODO
Add in Tempest for Eliza or something similar, so that people can calibrate
how well they've isolated their computer. (on my 49Mhz handheld radio I was able to pick up my computer monitor through 2-3 walls & about 10 meters) 

Next release I'd like to use use a USB storage device to write & send random
USB keyboard keystroke records across the USB bus.  USBsnoop runs on windows, and we can replay this traffic to disk. 
