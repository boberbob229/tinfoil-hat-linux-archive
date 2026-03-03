#!/bin/sh
# largely from code (c) 2001 Tito <farmatito@tiscalinet.it>
E3_PATH=/usr/bin/
case $0 in
   *ws|*wordstar)
   E3=e3ws
   ;;
   *vi)
   E3=e3vi
   ;;   
   *em|*emacs)
   E3=e3em
   ;;
   *pi|*pico)
   E3=e3pi
   ;;
   *nano)
    E3=e3pi
   ;;
   *ne|*nedit)
   E3=e3ne
   ;;
   *)
   echo "e3: improper emulation! ($0).Use emacs,wordstar,vi,pico,nedit"
   ;;
esac
if [ $@ =  ] 2>/dev/null
then
	$E3_PATH/$E3
else 
	for file in $@
	do
	$E3_PATH/$E3 $file
	done
fi 











