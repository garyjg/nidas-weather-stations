/*
 ********************************************************************
    Copyright by the National Center for Atmospheric Research

    $LastChangedDate: 2004-10-15 17:53:32 -0600 (Fri, 15 Oct 2004) $

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL: http://orion/svn/hiaper/ads3/dsm/class/RTL_DSMSensor.h $
 ********************************************************************

*/

#include <Aircraft.h>

#include <iostream>

using namespace dsm;
using namespace std;
using namespace xercesc;

// CREATOR_ENTRY_POINT(Aircraft)

Aircraft::Aircraft()
{
}

Aircraft::~Aircraft()
{
    for (std::list<DSMConfig*>::iterator it = dsms.begin();
    	it != dsms.end(); ++it) delete *it;
}

void Aircraft::fromDOMElement(const DOMElement* node)
	throw(atdUtil::InvalidParameterException)
{
    XDOMElement xnode(node);
    if (xnode.getNodeName().compare("aircraft"))
	    throw atdUtil::InvalidParameterException(
		    "Aircraft::fromDOMElement","xml node name",
		    	xnode.getNodeName());
		    
    if(node->hasAttributes()) {
	// get all the attributes of the node
	DOMNamedNodeMap *pAttributes = node->getAttributes();
	int nSize = pAttributes->getLength();
	for(int i=0;i<nSize;++i) {
	    XDOMAttr attr((DOMAttr*) pAttributes->item(i));
	    // get attribute name
	    // cerr << "attrname=" << attr.getName() << endl;
	    
	    // get attribute type
	    // cerr << "\tattrval=" << attr.getValue() << endl;
	}
    }

    DOMNode* child;
    for (child = node->getFirstChild(); child != 0;
	    child=child->getNextSibling())
    {
	if (child->getNodeType() != DOMNode::ELEMENT_NODE) continue;
	XDOMElement xchild((DOMElement*) child);
	const string& elname = xchild.getNodeName();
	cerr << "element name=" << elname << endl;

	if (!elname.compare("dsm")) {
	    DSMConfig* dsm = new DSMConfig();
	    dsm->fromDOMElement((DOMElement*)child);
	    addDSMConfig(dsm);
	}
	else if (!elname.compare("server")) {
	    DSMServer* server = new DSMServer();
	    server->fromDOMElement((DOMElement*)child);
	    addServer(server);
	}
    }
}

DOMElement* Aircraft::toDOMParent(DOMElement* parent) throw(DOMException) {
    DOMElement* elem =
        parent->getOwnerDocument()->createElementNS(
                (const XMLCh*)XMLStringConverter("aircraft"),
			DOMable::getNamespaceURI());
    parent->appendChild(elem);
    return toDOMElement(elem);
}
DOMElement* Aircraft::toDOMElement(DOMElement* node) throw(DOMException) {
    return node;
}

/**
 * Look for a server on this aircraft that either has no name or whose
 * name matches hostname.  If none found, remove any domain names
 * and try again.
 */
DSMServer* Aircraft::findServer(const string& hostname) const
{
    DSMServer* server = 0;
    for (list<DSMServer*>::const_iterator si=servers.begin();
	si != servers.end(); ++si) {
	DSMServer* srvr = *si;
	if (srvr->getName().length() == 0 ||
	    !srvr->getName().compare(hostname)) {
	    server = srvr;
	    break;
	}
    }
    if (server) return server;

    // Not found, remove domain name, try again
    int dot = hostname.find('.');
    for (list<DSMServer*>::const_iterator si=servers.begin();
	si != servers.end(); ++si) {
	DSMServer* srvr = *si;
	const std::string& sname = srvr->getName();
	int sdot = sname.find('.');
	if (!sname.compare(0,sdot,hostname,0,dot)) {
	    server = srvr;
	    break;
	}
    }
    return server;
}
