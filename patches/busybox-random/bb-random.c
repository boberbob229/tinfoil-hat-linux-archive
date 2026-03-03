/* vi: set sw=4 ts=4: */
/*
 * Mini randomm implementation for busybox
* Useful for making shell scripts do random actions. 
*
 *
 * Copyright (C) 2002 Anonymous
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "busybox.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>


extern int random_main(int argc, char **argv)
{
	int status = 0;
	int opt;
	int count = 0;
	ssize_t fill =0;
    char alpha[65] = 
/* to be base 64 compatible, the last character should be "/", but
  that's not filename safe, and I use this to generate random filenames
*/
	  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
	struct stat statbuf;
	int uf =-1;
	unsigned char c;


  if (stat("/dev/urandom", &statbuf) == 0 && S_ISCHR(statbuf.st_mode)) {
	uf= open("/dev/urandom", O_RDONLY );
	if ( uf > -1 ){
       for ( count=0 ; count < 3; count++) {
		 fill=read ( uf, &c, 1);
		 if (fill > 0) { break; } 
		}
	
	 if ( fill != 1 ) { return -1; }
	}
  } else {
	return -1;
  }

	while ((opt = getopt(argc, argv, "rbadnh")) != -1) {
		switch (opt) {
 		case 'r': /* raw mode */
			printf ( "%s\n" , &c );
			break;
		case 'd': /* Decimal 0-9*/
			c = c % 10;
			printf ( "%d\n", (int) c );
			break;
		case 'n': /* int from 0-254 */
			printf ( "%d\n", (int) c );
			break;
		case 'b': /* binary 1 or 0 */
		    if ( c < 128) { 
				printf ("1\n");
			} else {
				printf ("0\n");
			}
			break;	
		case 'a': /* one out of 64 filename safe characters only */
			c = c % 64;
			alpha[(c+1)]='\0';
			printf ("%s\n", &alpha[c]);
			break;
		case 'h': /* one hex number */
			c = c % 16;
			printf ("%X\n", (int) c );
			break;
		default:
			show_usage();
		}
	}

	status = 1;
	return status;
}
