/*
 ******************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$

 ******************************************************************
*/

#include <nidas/dynld/raf/Arinc_IRS.h>
//#include <iostream>

using namespace std;
using namespace nidas::dynld::raf;

namespace n_u = nidas::util;

Arinc_IRS::Arinc_IRS() :
  _irs_thdg_corr(0.0), _irs_ptch_corr(0.0), _irs_roll_corr(0.0)
{
#ifdef DEBUG
  err("");
#endif
}

void Arinc_IRS::fromDOMElement(const xercesc::DOMElement* node)
  throw(n_u::InvalidParameterException)
{
  DSMArincSensor::fromDOMElement(node);
  XDOMElement xnode(node);

  // parse attributes...
  if(node->hasAttributes()) {

    // get all the attributes of the node
    xercesc::DOMNamedNodeMap *pAttributes = node->getAttributes();
    int nSize = pAttributes->getLength();

    for(int i=0;i<nSize;++i) {
      XDOMAttr attr((xercesc::DOMAttr*) pAttributes->item(i));

      // parse attributes...
      if (!attr.getName().compare("irs_thdg_corr")) {
        istringstream ist(attr.getValue());
        ist >> _irs_thdg_corr;
        if ( ist.fail() )
          throw n_u::InvalidParameterException(getName(),
	  	attr.getName(),attr.getValue());
#ifdef DEBUG
        err("_irs_thdg_corr = %f", _irs_thdg_corr);
#endif
      }
      else if (!attr.getName().compare("irs_ptch_corr")) {
        istringstream ist(attr.getValue());
        ist >> _irs_ptch_corr;
        if ( ist.fail() )
          throw n_u::InvalidParameterException(getName(),
	  	attr.getName(),attr.getValue());
#ifdef DEBUG
        err("_irs_ptch_corr = %f", _irs_ptch_corr);
#endif
      }
      else if (!attr.getName().compare("irs_roll_corr")) {
        istringstream ist(attr.getValue());
        ist >> _irs_roll_corr;
        if ( ist.fail() )
          throw n_u::InvalidParameterException(getName(),
	  	attr.getName(),attr.getValue());
#ifdef DEBUG
        err("_irs_roll_corr = %f", _irs_roll_corr);
#endif
      }
    }
  }
}
