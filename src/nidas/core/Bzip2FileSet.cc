/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate: 2009-03-26 22:35:58 -0600 (Thu, 26 Mar 2009) $

    $LastChangedRevision: 4548 $

    $LastChangedBy: maclean $

    $HeadURL: http://svn.eol.ucar.edu/svn/nidas/trunk/src/nidas/core/FileSet.cc $
 ********************************************************************

*/

#ifdef HAS_BZLIB_H

#include <nidas/core/Bzip2FileSet.h>

using namespace nidas::core;
using namespace std;

namespace n_u = nidas::util;

Bzip2FileSet::Bzip2FileSet(): FileSet(new nidas::util::Bzip2FileSet())
{
     _name = "Bzip2FileSet";
}

/* Copy constructor. */
Bzip2FileSet::Bzip2FileSet(const Bzip2FileSet& x):
    	FileSet(x)
{
}

void Bzip2FileSet::fromDOMElement(const xercesc::DOMElement* node)
	throw(n_u::InvalidParameterException)
{
    FileSet::fromDOMElement(node);

    XDOMElement xnode(node);
    // const string& elname = xnode.getNodeName();
    if(node->hasAttributes()) {
	// get all the attributes of the node
        xercesc::DOMNamedNodeMap *pAttributes = node->getAttributes();
        int nSize = pAttributes->getLength();
        for(int i=0;i<nSize;++i) {
            XDOMAttr attr((xercesc::DOMAttr*) pAttributes->item(i));
            // get attribute name
            const std::string& aname = attr.getName();
            // const std::string& aval = attr.getValue();
	    if (aname == "compress");
	}
    }
}
#endif
