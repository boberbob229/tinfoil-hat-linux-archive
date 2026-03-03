/* gpggrid : wrapper for gpg that lets you enter in the password    
 * optionally stores passphrase in /tmp/ . This is fine if /tmp
 * is an encrypted ramdisk and this is a single user machine. 
*/

/*
Copyright (c) 2002 Anonymous  (anonymous @ nameless.cultists.net)
                   Morten Poulsen <morten@afdelingp.dk>
All rights reserved. 

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:                                                  
* Redistributions of source code must retain the above copyright notice, this 
list of conditions and the following disclaimer.                          
* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.                                                     
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT  NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                         
*/

/* CHANGES
* 1.1 release,  code was substantially improved by Morten Poulsen. 
* User options made somewhat clearer. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FILENAME "/tmp/P"
#define PSIZE 256  /* max passphrase size */

#define OUT 0
#define IN  1

/* I'm only allowing characters that most people might 
   actually know what to do with:
   EOT, BEL &etc are out as is newline, which leaves us with ASCII
   8-10,32-128 
   High ascii could be added, but I haven't checked for nastys
*/

/* 94 plus 5 unprintables */
static char printable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789{|}~[\\]^_`!\"#$%&\'()*+,-./:;<=>?@";
/* ASCII order="!\"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~"; */

char column[26];
char row[9];
char passphrase[PSIZE+2];
char SHOWPF=0;

void buildgrid(char *rand) {
  int i, r;

  printf ("   ");
  for (i=0; i<26; i++) {
    column[i] = printable[0 + ((i+rand[0]) % 26)];
    printf("%c ", column[i]);
  }
  printf ("\n");

  for (i=0; i<9; i++) {
    row[i] = printable[26 + ((i+rand[1]) % 26)];
  }

  r = 0;
  for (i=0; i<strlen(printable); i++) {
    if ((i % 26) == 0) {
      printf ("\n"); 
      printf ("%c  ", row[r++]);
    }
    printf ("%c ", printable[i]); 
  }

  /* UI is hard... Separate lines for these */
  printf ("\n%c   Backspace", row[r++]);
  printf ("\n%c   Tab", row[r++]);
  printf ("\n%c   Enter (stop entering characters)", row[r++]);
  printf ("\n%c   Space", row[r++]);
  printf ("\n%c   Delete\n\n", row[r++]);
}


int getcoords (char *key) {
  char *count;
  int i;
  int x = 0, y = 0;
  char buffer[PSIZE];
  int M = 0;
  int xok = 0, yok = 0;

  do {
    printf("Enter vertical row (lowercase) then horizontal column (UPPERCASE): ");
    count = fgets(buffer, PSIZE, stdin);

    if (count) {
      for (i=0; i<9; i++) {
        if (buffer[0] == row[i]) { 
          y = i; 
          yok = 1;
          break;
        }
      }
      for (i=0; i<26; i++) {
        if (buffer[1] == column[i]) { 
          x = i;
          xok = 1;
          break;
        }
      }
    }

  } while (!yok);

  /* Be nice to people who accidentally press enter too early */
  while (!xok) {
    printf("Please enter horizontal column (UPPER CASE) grid letter: ");
    count = fgets(buffer, PSIZE, stdin);

    if (count) {
      for (i=0; i<26; i++) {
        if (buffer[0] == column[i]) { 
          x = i;
          xok = 1;
          break;
        }
      }
    }
  }

  M  = 26 * y + x;
 
  if (M > 93)  {
    switch (M/26) {
      case 4:
        strcpy(key, "Backspace");
        return 2;
      case 5:
        strcpy(key, "\t");
        return 1;
      case 6:
        strcpy(key, "\n"); 
        return 3;
      case 7:
        strcpy(key, " ");
        return 1;
      case 8:
        strcpy(key, "Delete");
        return 2;
      default:    
        strcpy(key, "ERR");
        return -1;
    }

  } else {
    *key = printable[M];
/*    printf ("%d You seem to have chosen %c,%c ,which is %c\n",M, y,x, *key);*/
    return 1;
  }
} 


int getpassphrase() {
  char x;
  unsigned char crand[3];
  extern char passphrase[PSIZE+2];
  int check, c;
  int rs=-1;
  char thischar[12];
  char *curletter;
  struct stat statbuf;
  extern char SHOWPF;
  
  curletter = passphrase;

  /* initialize random source */
  if (stat("/dev/urandom", &statbuf) != 0 || !S_ISCHR(statbuf.st_mode)) {
    perror ("Can't find random source");
    exit(EXIT_FAILURE);
  } else {
    rs = open("/dev/urandom", O_RDONLY);
    if (rs < 0) {
      perror("Can't open random source");
      exit(EXIT_FAILURE);
    } 
  }

  x=0; c=0;
  while (x != 3) {
    printf("\n\n\n\n\n");

    if (SHOWPF == 2) { 
      printf(">>%s\n\n", passphrase);
    }

    check = read(rs, crand, 2);
    crand[0] = (crand[0] % 26);
    crand[1] = (crand[1] % 26);
    buildgrid(crand);

    x = getcoords(thischar);

    switch (x) {
      case 1: 
        /* if not 257 */   
        *curletter = *thischar;
        if (SHOWPF == 1) {
           printf ("Letter was %c\n\n", *curletter);
        }
        curletter++;
        break;
      case -1:
        printf ("please try again\n");
        break;
      case 2: /* DEL or backspace */  
        if (curletter >= passphrase) {
          curletter--;
          *curletter = '\0';
        } 
        break;
      case 3: /* Enter */
  /* should truncate passphrase at PSIZE.   */
        return 1;
    }
  }  
  return 0;
}


int main (int argc, char **argv) {
  extern char passphrase[PSIZE+2];
  char buffer[PSIZE+4];
  char *count;
  ssize_t foo;
  extern char SHOWPF;
  char fd_name[6];
  int i, fd_out;
  char *exec_argv[(argc+4)]; /* BUG: hardcoded, but should be enough */
  struct stat statbuf;
  int p[2];

  memset(&passphrase, 0, sizeof(passphrase));
  memset(&fd_name, 0, sizeof(fd_name));
 
  /* check if we already have a cached passphrase */
  if ((fd_out = open(FILENAME, O_RDONLY)) > 0) {
    printf("Using stored passphrase.\n");
    read(fd_out, passphrase, (PSIZE+1));
    /* We don't need to check for errors. If it fails, the user is asked for
       passphrase anyway. */
    close(fd_out);
  }

  if (strlen(passphrase) == 0) {
    /* Ask if we want to display the passphrase */
    printf("By default, you will see the last letter you entered. Do you\n");
    printf("want to display the entire passphrase on the screen instead?\n");
    printf("select y for full passphrase, n for no feedback, d for default\n");
    printf(" [y/n/D]\n");

    count = fgets( buffer, PSIZE, stdin);
    switch (buffer[0]) {
      case 'y':
      case 'Y':
        SHOWPF = 2;
        break;
      case 'n':
      case 'N':
        SHOWPF = 0;
        break;
      default:
        SHOWPF = 1;
        break;
    }

    getpassphrase();
 
    /* Ask if we want to save the passphrase */
    printf("\n\nGPGgrid can store your passphrase on the RAMdisk and re-use\n");
    printf("it later. This saves a lot of time, but does leave the passphrase vulnerable\n");
    printf("until reboot.\n"); 
    printf("Do you want to store the passphrase in /tmp ? [Y/n]\n");
    count = fgets(buffer, PSIZE, stdin);

    if (buffer[0] != 'n' && buffer[0] != 'N') {

      /* write to disk */
      printf ("Writing passphrase to disk.\n" );
      if ((stat(FILENAME, &statbuf) == 0) && (!S_ISREG(statbuf.st_mode))) {
        printf(FILENAME " is not a regular file\n");
      } else {
        if ((fd_out = open(FILENAME, O_RDWR|O_CREAT|O_TRUNC, 0600)) == -1) {
          perror("can't open passphrase file for writing") ;
        } else {
          foo = write(fd_out, passphrase, strlen(passphrase));
          if (foo != strlen(passphrase)) {
            perror("can't write out the passphrase"); 
          }
          close (fd_out) ;
        }
      }
    }
  }

  if (SHOWPF == 2)  { 
    printf("Final result: %s\n", passphrase); 
  }
 
  /* OK, we're ready to run GPG */

  if (pipe(p) == -1) {
    perror("unable to create pipe");
    exit(EXIT_FAILURE);
  }

  switch (fork()) {

    case -1:  /* failure */
      perror("unable to fork");
      exit(EXIT_FAILURE);

    case 0:  /* child */
      close(p[OUT]);
      write(p[IN], passphrase, strlen(passphrase));
      close(p[IN]);
      exit(EXIT_SUCCESS);

    default:  /* parent */
      close(p[IN]);
      sprintf(fd_name, "%d", p[OUT]);
      exec_argv[0] = "gpg";
      exec_argv[1] = "--passphrase-fd";
      exec_argv[2] = fd_name;
      for (i=1; i<=argc; i++) {  /* <= to get the NULL pointer too */
        exec_argv[i+2] = argv[i];
      }
      execvp("gpg", exec_argv);
      exit(EXIT_FAILURE);
  }

  /* never reached */
  return EXIT_FAILURE;

}
