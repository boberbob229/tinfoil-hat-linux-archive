#!/bin/sh
#randomly do stuff with PGP. 

dir="/tmp/new"
# file to use for frequent encryption. use /proc/kcore if you're very bold. 
in="/dev/ram1"
rk="/tmp/gnupg"
hdir="--batch --no-tty  --homedir $dir "

mkdir $dir
cat  >/tmp/new/keyopt <<EOF
Key-Type: DSA
Key-Length: 512
Subkey-Type: ELG-E
Subkey-Length: 512
Name-Real: $$ 
Name-Comment: test
Name-Email: test@example.com
Expire-Date: 1d
%commit
EOF

gpg --quick-random $hdir --gen-key -a /tmp/new/keyopt >/dev/null 2>&1 

while [ 1 ]
do
  sleep 2 
  x=`/bin/random -a`;
	# thrash disk, encrypt strings	 to new key
  find /mnt/ -name $x* 2>/dev/null | gpg -ear $$ >/dev/null 2>&1 

C=`/bin/random -n`

if [ "$C" -lt "64" ]; 
then 
	gpg --quick-random -q  $hdir --gen-key -a /tmp/new/keyopt >/dev/null 2>&1 

elif [ "$C" -lt "0" ];
then 
#BUG: can't get there from here
# sign our private keys: can't be done w/o patching GPG
 T=`gpg  $hdir --list-keys --with-colon |grep "pub"|cut -f5 -d":" |xargs  `
 for c in  $T 
 do
  gpg $hdir -o - --sign-key $c
 done

elif [ "$C" -lt "100" ];
then
# find a random file & encrypt it to self
  x=`/bin/random -a`;
find /mnt/ -name $x* 2>/dev/null |head -n2  |xargs gpg -ear $$ >/dev/null 2>&1 

elif [ "$C" -lt "128" ];
then 
# sign & stuff to self, then decrypt
  (gpg $hdir -o - -asr $$ -e $in  | gpg $hdir --decrypt >/dev/null )
  
elif [ "$C" -lt "192" ]; 
then
# find recipients in public ring & encrypt files to them
 T=`gpg  --batch --no-tty --homedir $rk --list-keys --with-colon |grep "pub"|cut -f5 -d":" |xargs  `
 for c in  $T 
 do
  gpg --always-trust -o - $hdir --keyring $rk/pubring.gpg -r $c -ase $in >/dev/null 2>&1 
 done

else
	# Non RSA encryption
  md5sum $in | gpg  $hdir --passphrase-fd 0 -o - -c $in  >/dev/null 2>&1
fi

done 
