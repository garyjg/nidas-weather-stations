/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#include <nidas/core/DSMCatalog.h>

#include <iostream>

using namespace nidas::core;
using namespace std;

namespace n_u = nidas::util;

DSMCatalog::DSMCatalog()
{
}

DSMCatalog::~DSMCatalog()
{
}

void DSMCatalog::fromDOMElement(const xercesc::DOMElement* node)
	throw(n_u::InvalidParameterException)
{
    XDOMElement xnode(node);
    
    if (xnode.getNodeName() != "dsmcatalog")
	    throw n_u::InvalidParameterException(
		    "DSMCatalog::fromDOMElement","xml node name",
		    	xnode.getNodeName());
		    
    xercesc::DOMNode* child;
    for (child = node->getFirstChild(); child != 0;
	    child=child->getNextSibling())
    {
	if (child->getNodeType() != xercesc::DOMNode::ELEMENT_NODE) continue;
	XDOMElement xchild((xercesc::DOMElement*) child);
	const string& elname = xchild.getNodeName();
	// cerr << "DSMCatalog: child element name=" << elname << endl;

	if (elname == "dsm") {
	    const string& id = xchild.getAttributeValue("ID");
	    if(id.length() > 0) {
		map<string,xercesc::DOMElement*>::iterator mi =
			find(id);
		if (mi != end() && mi->second != (xercesc::DOMElement*)child)
		    throw n_u::InvalidParameterException(
			"DSMCatalog::fromDOMElement",
			"duplicate sensor in catalog, ID",id);
		insert(make_pair<string,xercesc::DOMElement*>(id,(xercesc::DOMElement*)child));

		/*
		cerr << "sensorCatalog.size=" << size() << endl;
		for (mi = begin(); mi != end(); ++mi)
		    cerr << "map:" << mi->first << " " << hex << mi->second <<
		    	dec << endl;
		*/
	    }
        }
	else throw n_u::InvalidParameterException(
			"DSMCatalog::fromDOMElement",
			"unrecognized element",elname);
    }
}

