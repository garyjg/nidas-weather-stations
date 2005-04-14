/* Parsci.cc
 
   Parsci digital Pressure  class.
 
   Original Author: Mike Spowart
   Copyright 2005 UCAR, NCAR, All Rights Reserved
 
   Revisions:
 
*/

#include <Parsci.h>

/******************************************************************************
** Public Functions
******************************************************************************/

Parsci::Parsci () 
{ 
// Constructor. 

  ptog = 0;
  gtog = 0;
  idx = 0;

}

/*****************************************************************************/
const char *Parsci::buffer()
{
  return((const char*)&parsci_blk[gtog]);
}
/*****************************************************************************/
void Parsci::parser(int len)
{
  if (idx > 24)
    return;
  sscanf (&buf[5], "%f",parsci_blk[ptog].press[idx++]);

}
/*****************************************************************************/
void Parsci::secondAlign()

// This routine is to be called at each 1 second clock tick. The Parsci_blk
// buffers are toggled.
{
  gtog = ptog;
  ptog = 1 - ptog;
  idx = 0;
}

