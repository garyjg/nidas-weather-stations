/* Parsci.h

   Parascientific digital pressure instrument class.
 
   Original Author: Mike Spowart
   Copyright by the National Center for Atmospheric Research
 
   Revisions:
 
*/

/* #include <logLib.h> */
#include <string.h>
#include <stdio.h>
#include <dsmctl.h>
#include <header.h>
#include <messageDefs.h>

#define PARSCI_STR       "PARSCI"

class Parsci {

public:
  Parsci ();
  const char* buffer();
  void parser();
  void secondAlign();

  const char buf[1000];

  struct Parsci_blk {
    float press[5];                               /* Digital pressure */
  };
  typedef struct Parsci_blk Parsci_blk;

private:
  Parsci_blk parsci_blk[TOG];			// local data block
  int ptog;
  int gtog;
  int idx;
};

