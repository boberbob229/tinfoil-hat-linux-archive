/* gpggrid : wrapper for gpg that lets you enter in the password    
 * optionally stores passphrase in /tmp/ . This is fine if /tmp
 * is an encrypted ramdisk and this is a single user machine. 
 * Code is ugly, Error checking is missing.  I != coder. If you are, fix it. 
 * TODO:
 * Get by with less file opening & closing while still failing nicely?
*/
/*
Copyright (c) 2002 Anonymous  (anonymous @ nameless.cultists.net)
All rights reserved. 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:                                                  
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.                          
* Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.                                                     
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT  NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES                                   
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                         
*/





#define FILENAME "/tmp/P"
/*max passphrase size */
#define PSIZE 256 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

/* I'm only allowing characters that most people might 
 actually know what to do with:
EOT, BEL &etc are out as is newline, which leaves us with ASCII
8-10,32-128 
High ascii  could be added, but I haven't checked for nastys
*/

/* 94 plus 5 unprintables */
static char printable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{|}~[\\]^_`!\"#$%&\'()*+,-./:;<=>?@";
/* ASCII order="!\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; */

char row[27];
char column[10];
char passphrase[PSIZE+2];
char SHOWPF=0;

int buildgrid ( char * rand ) {
  unsigned char a, b, c ;
  char  * xzero, * yzero;
  /* char check;*/
  int i;
  a = b = c =0;

/* print the static header */ 
  xzero = &printable[0] + rand[0];
  i = &printable[26] - xzero;
  snprintf(row, (size_t) (i+1), "%s", xzero);
  snprintf(&row[i], (size_t) (rand[0] +1), "%s", &printable[0]);
  printf ("\n   ");
  for (i=0;i < 26; i++) {
    printf("%c ",row[i]);
  }
  printf ("\n");

  yzero= &printable[26] +rand[1];
  i = &printable[52] - yzero ;
  if (i > 9) { i=9; }
  snprintf(column, (size_t) (i+1), "%s", yzero);
  if (i < 9) {
    snprintf(&column[i], (size_t) (10 -i ), "%s", &printable[26]);
  }
  for (i = 0; i < 94; i++) { /*sizeof printable */
    if ( (i%26) == 0) {
      printf ("\n"); 
      printf ("%c  ", column[c]);
      c++;
    }
    printf ("%c ", printable[i]); 
  }
 /* UI is hard... Separate lines for these */
  printf ("\n%c   Backspace ", column[c++]);
  printf ("\n%c   Tab ", column[c++]);
  printf ("\n%c   Enter (stop entering characters)  ", column[c++]);
  printf ("\n%c   Space ", column[c++]);
  printf ("\n%c   Delete\n\n", column[c++]);
  return 1;
}


int  getcords ( char * key) {
char * count, *here;
int i,ii;
char x,y;
char buffer[PSIZE];
int M=0;

 while ( y==0) {
   printf("Enter grid column (lowercase) then row (uppercase):");
   count = fgets( buffer, PSIZE, stdin);
   if( count != NULL) {
     for (i=0;i<9;i++){
       if ( column[i] == count[0]) { 
	 for (ii=0;ii<26;ii++){ /*or strstr*/
	   if ( row[ii] == count[1]) { 
	     x=count[1];
	     break;
	   }
	 }
	 y=count[0]; 
       }
     }
   }
 }

 while ( x==0) {
   printf ("\nplease enter row (CAPS) grid letter: ");
   count = fgets( buffer, PSIZE, stdin);
   if( count != NULL) {
     for (i=0;i<26;i++){
       if ( row[i] == count[0]) {x=count[0];}
     }
   }
 }

/* figure out how many rows down we are */
 here = strchr(&column[0], (int) y);
 M  = 26 * ( here - &column[0]);
 here = strchr(row,(int)x);
 M  = M + (here - &row[0]);
 
 if (M > 93)  {
   switch (M/26) {
   case 4:
     strcpy(key,"Backspace");
     return 2;
     break;
   case 5:
     sprintf(key,"\t");
     return 1;
     break;
   case 6:
     strcpy(key,"\n"); /* good idea?, I doubt it */
     return 3;
     break;
   case 7:
     sprintf(key," ");
     return 1;
     break;
   case 8:
     strcpy(key,"Delete");
     return 2;
     break;
   default:	
     strcpy(key,"ERR");
     return -1;
     break;
   }
 } else {
   *key = printable[M];
/*	printf ("%d You seem to have chosen %c,%c ,which is %c\n",M, y,x, *key);*/
   return 1;
 }
} 

void fillrand ( char * randbuf, int rs) {
  printf( "your random is %s", randbuf);
}

int getpassphrase () {
char x;
unsigned char crand[3];
extern char passphrase[PSIZE+2];
int check, c;
int rs=-1;
char thischar[12];
char * curletter;
struct stat statbuf;
extern char SHOWPF;
curletter=passphrase;

/* initialize random source */
 if (stat("/dev/urandom", &statbuf) == 0 && S_ISCHR(statbuf.st_mode)) {
   rs= open("/dev/urandom", O_RDONLY );
   if ( rs < 0 ){
     perror ("Can't open random source ");
     exit;
   } 
 } else {
   perror ("Can't find random source ");
 }

/*add count to check size is less than PSIZE */
 x=0; c=0;
 while (x != 3) {
   if (SHOWPF == 2) { 
     printf("\n>>%s\n",passphrase);
   }
/* reduce to 1 byte */
   check=read ( rs, crand,2);
   crand[0] = (crand[0] %26 );
   crand[1] = (crand[1] %26 );
   buildgrid (&crand[0]);
   x =  getcords (&thischar[0]);
   
   switch (x) {
   case 1: 
     /* if not 257 */   
     *curletter=thischar[0];
     printf ("\n\n\n\n\n");
     if (SHOWPF == 1) {
       printf ("Last letter was %c \n", *curletter);
     }
     curletter++;
     break;
   case -1:
     printf ("please try again \n");
     break;
   case 2: /* DEL or backspace */  
     if ( curletter >= &passphrase[0]) {
       curletter--;
       *curletter='\0';
     } 
     break;
   case 3: /* Enter */
     /* log size of passphrase here */ 
     return 1;
   }
 }  
 return 0;
}


int main (int argc, char *argv[]) {
extern char passphrase[PSIZE+2];
char buffer[PSIZE+4];
char * count;
ssize_t pushed, foo;
extern char SHOWPF;
char fd_name[6];
int i, fd_out, fd_gpg;
char *exec_argv[(argc+4)]; /* BUG: should be enough*/
struct stat statbuf;

 memset (&passphrase,NULL, sizeof(passphrase));
 memset (&fd_name,NULL, sizeof(fd_name));

/* check if we already have a cached passphrase */
 if ((fd_out= open (FILENAME,O_RDONLY, 0600)) == -1 ) {
   perror ("Damn, can't open passphrase file") ;
 } else {
   pushed = read ( fd_out, passphrase, (PSIZE+1) );
   /* BUG, don't find 1 char short */
   if ( pushed  <0 ) {
     perror ("Whoah, writing went strange:");
   }
   close (fd_out);
 }

 if (strlen(passphrase) ==0) {
   perror ("stat failed:");
/* Ask if we want to display the passphrase */
   printf("By default, you will see the last letter you entered. Do you \nwant to display the entire passphrase on the screen instead? \n");
   printf ("select y for full passphrase, b for no feedback, n for default\n");
   printf (" [y/b/N]\n");

   count = fgets( buffer, PSIZE, stdin);
   switch (count[0]) {
   case 'y':
   case 'Y':
     SHOWPF = 2;
     break;
   case 'b':
   case 'B':
     SHOWPF = 0;
     break;
   default:
     SHOWPF = 1;
     break;
   }
   getpassphrase();
 
/* Ask if we want to save the passphrase */

   printf ("\n\n GPGgrid can store your passphrase on the RAMdisk and re-use \n it later. This saves a lot of time, but does leave the passphrase vulnerable \n  until reboot.\n"); 

   printf("Do you want to store the passphrase in RAM ? [Y/n]\n");
   count = fgets( buffer, PSIZE, stdin);
   if ( count[0] != 'n' && count[0] != 'N') {
     /* write to disk */
     printf ("writing to disk\n" );
     if (stat(FILENAME, &statbuf) == 0 && ( S_ISREG(statbuf.st_mode) )) {
       if ((fd_out= open (FILENAME,O_RDWR | O_CREAT |O_TRUNC , 0600)) == -1 ) {
	 perror ("Damn, can't write passphrase file") ;
       }
       /* BUG: truly lame. Need a write wrapper program */
       pushed=strlen(passphrase);
       foo= write(fd_out, passphrase, strlen(passphrase));
       if (foo != pushed ) {
	 perror ("can't write out the passphrase: "); 
       }
       close (fd_out) ;
     }
   }
 }

 if (SHOWPF == 2)  { 
   printf("Final result:: %s\n", &passphrase[0]); 
 }
 
/* OK, we're ready to run GPG */
 printf ("Running GPG with %u args \n", argc);
 if ((fd_gpg= open (FILENAME,O_RDONLY, 0600)) == -1 ) {
   perror ("Damn, can't read passphrase file") ;
 }
 sprintf(&fd_name[0], "%d",fd_gpg);

 exec_argv[0]="gpg";
 exec_argv[1]="--passphrase-fd";
 exec_argv[2]=&fd_name[0] ;
 for (i= 1; i <= argc ; i++) {
   exec_argv[i+2]=argv[i];
 }
 execvp ("gpg", exec_argv );
 return -1;
}
