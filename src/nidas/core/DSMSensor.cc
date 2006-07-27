
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#include <nidas/core/DSMSensor.h>
#include <nidas/core/Project.h>
#include <nidas/core/DSMConfig.h>
#include <nidas/core/Site.h>
#include <nidas/core/NidsIterators.h>

#include <nidas/core/dsm_sample.h>
#include <nidas/core/SamplePool.h>
#include <nidas/util/Logger.h>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <string>
#include <set>

using namespace std;
using namespace nidas::core;

namespace n_u = nidas::util;

/* static */
bool DSMSensor::zebra = false;

DSMSensor::DSMSensor() :
    iodev(0),scanner(0),dsm(0),id(0),
    rawSampleTag(0),
    height(floatNAN),
    latency(0.1)	// default sensor latency, 0.1 secs
{
}


DSMSensor::~DSMSensor()
{

    for (set<SampleTag*>::const_iterator si = sampleTags.begin();
    	si != sampleTags.end(); ++si) {
	delete *si;
    }
    delete rawSampleTag;
    delete scanner;
    delete iodev;

    map<std::string,Parameter*>::const_iterator pi;
    for (pi = parameters.begin(); pi != parameters.end(); ++pi)
	delete pi->second;
}


void DSMSensor::addSampleTag(SampleTag* val)
    throw(n_u::InvalidParameterException)
{
    sampleTags.insert(val);
    constSampleTags.insert(val);
}

SampleTagIterator DSMSensor::getSampleTagIterator() const
{
    return SampleTagIterator(this);
}

VariableIterator DSMSensor::getVariableIterator() const
{
    return VariableIterator(this);
}

/**
 * Fetch the DSM name.
 */
const std::string& DSMSensor::getDSMName() const {
    static std::string unk("unknown");
    if (dsm) return dsm->getName();
    return unk;
}

/*
 * If location is an empty string, return DSMConfig::getLocation()
 */
const std::string& DSMSensor::getLocation() const {
    if (location.length() == 0 && dsm) return dsm->getLocation();
    return location;
}

void DSMSensor::setHeight(const string& val)
{
    heightString = val;
    if (heightString.length() > 0) {
	float h;
	istringstream ist(val);
	ist >> h;
	if (ist.fail()) height = floatNAN;
	else if (!ist.eof()) {
	    string units;
	    ist >> units;
	    if (!ist.fail()) {
		if (units == "cm") h /= 10.0;
		else if (units != "m") h = floatNAN;
	    }
	    height = h;
	}
	setSuffix(string(".") + heightString + getSiteSuffix());
    }
    else {
	height = floatNAN;
	setSuffix(getSiteSuffix());
    }
}

void DSMSensor::setHeight(float val)
{
    height = val;
    if (! isnan(height)) {
	ostringstream ost;
	ost << height << 'm';
	heightString = ost.str();
	setSuffix(string(".") + heightString + getSiteSuffix());
    }
    else setSuffix(getSiteSuffix());
}

void DSMSensor::setDepth(const string& val)
{
    depthString = val;
    if (depthString.length() > 0) {
	float d;
	istringstream ist(val);
	ist >> d;
	if (ist.fail()) height = floatNAN;
	else if (!ist.eof()) {
	    string units;
	    ist >> units;
	    if (!ist.fail()) {
		if (units == "cm") d /= 10.0;
		else if (units != "m") d = floatNAN;
	    }
	    height = -d;
	}
	setSuffix(string(".") + depthString + getSiteSuffix());
    }
    else {
	height = floatNAN;
	setSuffix(getSiteSuffix());
    }
}

void DSMSensor::setDepth(float val)
{
    height = -val;
    if (! isnan(height)) {
	ostringstream ost;
	ost << val * 10.0 << "cm";
	depthString = ost.str();
	setSuffix(string(".") + depthString + getSiteSuffix());
    }
    else setSuffix(getSiteSuffix());
}

/*
 * Add a parameter to my map, and list.
 */
void DSMSensor::addParameter(Parameter* val)
{
    map<string,Parameter*>::iterator pi = parameters.find(val->getName());
    if (pi == parameters.end()) {
        parameters[val->getName()] = val;
	constParameters.push_back(val);
    }
    else {
	// parameter with name exists. If the pointers aren't equal
	// delete the old parameter.
	Parameter* p = pi->second;
	if (p != val) {
	    // remove it from constParameters list
	    list<const Parameter*>::iterator cpi = constParameters.begin();
	    for ( ; cpi != constParameters.end(); ) {
		if (*cpi == p) cpi = constParameters.erase(cpi);
		else ++cpi;
	    }
	    delete p;
	    pi->second = val;
	    constParameters.push_back(val);
	}
    }
}

const Parameter* DSMSensor::getParameter(const std::string& name) const
{
    map<string,Parameter*>::const_iterator pi = parameters.find(name);
    if (pi == parameters.end()) return 0;
    return pi->second;
}

/*
 * Open the device. flags are a combination of O_RDONLY, O_WRONLY.
 */
void DSMSensor::open(int flags)
	throw(n_u::IOException,n_u::InvalidParameterException) 
{
    if (!iodev) iodev = buildIODevice();
    // cerr << "iodev->setName " << getDeviceName() << endl;
    iodev->setName(getDeviceName());

    n_u::Logger::getInstance()->log(LOG_NOTICE,
    	"opening: %s",getDeviceName().c_str());

    iodev->open(flags);
    if (!scanner) scanner = buildSampleScanner();
    scanner->init();
}

/*
 * Open the device. flags are a combination of O_RDONLY, O_WRONLY.
 */
void DSMSensor::close() throw(n_u::IOException) 
{
    n_u::Logger::getInstance()->log(LOG_INFO,
    	"closing: %s",getDeviceName().c_str());
    iodev->close();
}

void DSMSensor::init() throw(n_u::InvalidParameterException)
{
}

bool DSMSensor::receive(const Sample *samp) throw()
{
    list<const Sample*> results;
    process(samp,results);
    distribute(results);	// distribute does the freeReference
    return true;
}

/**
 * Default implementation of process just passes samples on.
 */
bool DSMSensor::process(const Sample* s, list<const Sample*>& result) throw()
{
    s->holdReference();
    result.push_back(s);
    return true;
}

void DSMSensor::printStatusHeader(std::ostream& ostr) throw()
{
  static char *glyph[] = {"\\","|","/","-"};
  static int anim=0;
  if (++anim == 4) anim=0;
  zebra = false;

  string dsm_name(getDSMConfig()->getName());
  string dsm_lctn(getDSMConfig()->getLocation());

    ostr <<
"<table id=\"sensor_status\">\
<caption>"+dsm_lctn+" ("+dsm_name+") "+glyph[anim]+"</caption>\
<tr>\
<th>name</th>\
<th>samp/sec</th>\
<th>byte/sec</th>\
<th>min&nbsp;samp<br>length</th>\
<th>max&nbsp;samp<br>length</th>\
<th>bad<br>timetags</th>\
<th>extended&nbsp;status</th>\
<tbody align=center>" << endl;	// default alignment in table body
}

void DSMSensor::printStatusTrailer(std::ostream& ostr) throw()
{
    ostr << "</tbody></table>" << endl;
}
void DSMSensor::printStatus(std::ostream& ostr) throw()
{
    string oe(zebra?"odd":"even");
    zebra = !zebra;
    ostr <<
        "<tr class=\"" << oe << "\"><td align=left>" <<
                getDeviceName() << ',' <<
		(getCatalogName().length() > 0 ?
			getCatalogName() : getClassName()) <<
		"</td>" << endl <<
    	"<td>" << fixed << setprecision(2) <<
		getObservedSamplingRate() << "</td>" << endl <<
    	"<td>" << setprecision(0) <<
		getObservedDataRate() << "</td>" << endl <<
	"<td>" << getMinSampleLength() << "</td>" << endl <<
	"<td>" << getMaxSampleLength() << "</td>" << endl <<
	"<td>" << getBadTimeTagCount() << "</td>" << endl;
}

/* static */
const string DSMSensor::getClassName(const xercesc::DOMElement* node)
    throw(n_u::InvalidParameterException)
{
    XDOMElement xnode(node);
    const string& idref = xnode.getAttributeValue("IDREF");
    if (idref.length() > 0) {
	Project* project = Project::getInstance();
	if (!project->getSensorCatalog())
	    throw n_u::InvalidParameterException(
		"sensor",
		"cannot find sensorcatalog for sensor with IDREF",
		idref);

	map<string,xercesc::DOMElement*>::const_iterator mi;
	mi = project->getSensorCatalog()->find(idref);
	if (mi == project->getSensorCatalog()->end())
		throw n_u::InvalidParameterException(
	    "sensor",
	    "sensorcatalog does not contain a sensor with ID",
	    idref);
	const string classattr = getClassName(mi->second);
	if (classattr.length() > 0) return classattr;
    }
    return xnode.getAttributeValue("class");
}

void DSMSensor::fromDOMElement(const xercesc::DOMElement* node)
    throw(n_u::InvalidParameterException)
{

    /*
     * The first time DSMSensor::fromDOMElement is called for
     * a DSMSensor, it is with the "actual" child DOMElement
     * within a <dsm> tag, not the DOMElement from the catalog entry.
     * We set the critical attributes (namely: id) from the actual
     * element, then parse the catalog element, then do a full parse
     * of the actual element.
     */
  
    setDSMId(getDSMConfig()->getId());

    XDOMElement xnode(node);

    // set id before scanning catalog entry
    const string& idstr = xnode.getAttributeValue("id");
    if (idstr.length() > 0) {
	istringstream ist(idstr);
	// If you unset the dec flag, then a leading '0' means
	// octal, and 0x means hex.
	ist.unsetf(ios::dec);
	unsigned long val;
	ist >> val;
	if (ist.fail())
	    throw n_u::InvalidParameterException(
		string("sensor on dsm ") + getDSMConfig()->getName(),
		"id",idstr);
	setShortId(val);
    }

    const string& idref = xnode.getAttributeValue("IDREF");
    // scan catalog entry
    if (idref.length() > 0) {
	Project* project = Project::getInstance();
	if (!project->getSensorCatalog())
	    throw n_u::InvalidParameterException(
		string("dsm") + ": " + getName(),
		"cannot find sensorcatalog for sensor with IDREF",
		idref);

	map<string,xercesc::DOMElement*>::const_iterator mi;

	mi = project->getSensorCatalog()->find(idref);
	if (mi == project->getSensorCatalog()->end())
		throw n_u::InvalidParameterException(
	    string("dsm") + ": " + getName(),
	    "sensorcatalog does not contain a sensor with ID",
	    idref);
	// read catalog entry
	setCatalogName(idref);
        fromDOMElement(mi->second);
    }

    // Now the main entry attributes will override the catalog entry attributes
    if(node->hasAttributes()) {
    // get all the attributes of the node
	xercesc::DOMNamedNodeMap *pAttributes = node->getAttributes();
	int nSize = pAttributes->getLength();
	for(int i=0;i<nSize;++i) {
	    XDOMAttr attr((xercesc::DOMAttr*) pAttributes->item(i));
	    const string& aname = attr.getName();
	    const string& aval = attr.getValue();
	    // get attribute name
	    if (aname == "devicename")
		setDeviceName(aval);
	    else if (aname == "id");	// already scanned
	    else if (aname == "IDREF");		// already parsed
	    else if (aname == "class") {
	        if (getClassName().length() == 0)
		    setClassName(aval);
		else if (getClassName() != aval)
		    n_u::Logger::getInstance()->log(LOG_WARNING,
		    	"class attribute=%s does not match getClassName()=%s\n",
			aval.c_str(),getClassName().c_str());
	    }
	    else if (aname == "location") setLocation(aval);
	    else if (aname == "latency") {
		istringstream ist(aval);
		float val;
		ist >> val;
		if (ist.fail())
		    throw n_u::InvalidParameterException("sensor",
		    	aname,aval);
		setLatency(val);
	    }
	    else if (aname == "height")
	    	setHeight(aval);
	    else if (aname == "depth")
	    	setDepth(aval);
	    else if (aname == "suffix")
	    	setSuffix(aval);
	}
    }
    
    if (getShortId() == 0) 
	throw n_u::InvalidParameterException(
	    getDSMConfig()->getName() + ": " + getName(),
	    "id is zero","");

    xercesc::DOMNode* child;
    for (child = node->getFirstChild(); child != 0;
	    child=child->getNextSibling())
    {
	if (child->getNodeType() != xercesc::DOMNode::ELEMENT_NODE) continue;
	XDOMElement xchild((xercesc::DOMElement*) child);
	const string& elname = xchild.getNodeName();

	if (elname == "sample") {
	    SampleTag* newtag = new SampleTag();
	    newtag->setDSM(getDSMConfig());
	    newtag->setDSMId(getDSMConfig()->getId());
	    newtag->setSensorId(getShortId());
	    newtag->fromDOMElement((xercesc::DOMElement*)child);

	    if (newtag->getSampleId() == 0) {
		delete newtag;
		throw n_u::InvalidParameterException(
		    getName(),"sample id invalid or not found","0");
	    }

	    set<SampleTag*>& stags = getncSampleTags();
	    set<SampleTag*>::const_iterator si = stags.begin();
	    for ( ; si != stags.end(); ++si) {
		SampleTag* stag = *si;
		// If a sample id matches a previous one (most likely
		// from the catalog) then update it from this DOMElement.
		if (stag->getSampleId() == newtag->getSampleId()) {
		    // update the sample with the new DOMElement
		    stag->setDSMId(getDSMConfig()->getId());
		    stag->setSensorId(getShortId());

		    stag->fromDOMElement((xercesc::DOMElement*)child);
		    
		    delete newtag;
		    newtag = 0;
		    break;
		}
	    }
	    if (newtag) addSampleTag(newtag);
	}
	else if (elname == "parameter") {
	    Parameter* parameter =
	    Parameter::createParameter((xercesc::DOMElement*)child);
	    addParameter(parameter);
	}
    }

    if (!rawSampleTag) {
	rawSampleTag = new SampleTag();
	rawSampleTag->setSampleId(0);
	rawSampleTag->setSensorId(getShortId());
	rawSampleTag->setDSMId(getDSMConfig()->getId());
	rawSampleTag->setDSM(getDSMConfig());
    }

    if (getSuffix().length() > 0)
    	rawSampleTag->setSuffix(getSuffix());

    const Site* site = getDSMConfig()->getSite();
    if (site) rawSampleTag->setStation(site->getNumber());

    // sensors in the catalog may not have any sample tags
    // so at this point it is OK if sampleTags.size() == 0.

    // Check that sample ids are unique for this sensor.
    // Estimate the rate of the raw sample as the max of
    // the rates of the processed samples.
    float rawRate = 0.0;
    set<unsigned long> ids;
    set<SampleTag*>& stags = getncSampleTags();
    set<SampleTag*>::const_iterator si = stags.begin();
    for ( ; si != stags.end(); ++si) {
	SampleTag* stag = *si;

	stag->setSensorId(getShortId());
	if (getSuffix().length() > 0)
	    stag->setSuffix(getSuffix());
	if (site) stag->setStation(site->getNumber());

	if (getShortId() == 0) throw n_u::InvalidParameterException(
	    	getName(),"id","zero or missing");

	pair<set<unsigned long>::const_iterator,bool> ins =
		ids.insert(stag->getId());
	if (!ins.second) {
	    ostringstream ost;
	    ost << stag->getId();
	    throw n_u::InvalidParameterException(
	    	getName(),"duplicate sample id", ost.str());
	}
	rawRate = std::max(rawRate,stag->getRate());
    }
    rawSampleTag->setRate(rawRate);
}

void DSMSensor::checkConfig()
    throw(n_u::InvalidParameterException)
{
    if (getDeviceName().length() == 0) 
	throw n_u::InvalidParameterException(
	    getDSMConfig()->getName() + ": " + getName(),
	    "no device name","");

}

xercesc::DOMElement* DSMSensor::toDOMParent(
    xercesc::DOMElement* parent)
    throw(xercesc::DOMException)
{
    xercesc::DOMElement* elem =
        parent->getOwnerDocument()->createElementNS(
                (const XMLCh*)XMLStringConverter("dsmconfig"),
			DOMable::getNamespaceURI());
    parent->appendChild(elem);
    return toDOMElement(elem);
}

xercesc::DOMElement* DSMSensor::toDOMElement(xercesc::DOMElement* node)
    throw(xercesc::DOMException)
{
    return node;
}

/* static */
string DSMSensor::replaceBackslashSequences(string str)
{
    unsigned int bs;
    for (unsigned int ic = 0; (bs = str.find('\\',ic)) != string::npos;
    	ic = bs) {
	bs++;
	if (bs == str.length()) break;
        switch(str[bs]) {
	case 'n':
	    str.erase(bs,1);
	    str[bs-1] = '\n';
	    break;
	case 'r':
	    str.erase(bs,1);
	    str[bs-1] = '\r';
	    break;
	case 't':
	    str.erase(bs,1);
	    str[bs-1] = '\t';
	    break;
	case '\\':
	    str.erase(bs,1);
	    str[bs-1] = '\\';
	    break;
	case 'x':	//  \xhh	hex
	    if (bs + 2 >= str.length()) break;
	    {
		istringstream ist(str.substr(bs+1,2));
		int hx;
		ist >> hex >> hx;
		if (!ist.fail()) {
		    str.erase(bs,3);
		    str[bs-1] = (char)(hx & 0xff);
		}
	    }
	    break;
	case '0':	//  \000   octal
	case '1':
	case '2':
	case '3':
	    if (bs + 2 >= str.length()) break;
	    {
		istringstream ist(str.substr(bs,3));
		int oc;
		ist >> oct >> oc;
		if (!ist.fail()) {
		    str.erase(bs,3);
		    str[bs-1] = (char)(oc & 0xff);
		}
	    }
	    break;
	}
    }
    return str;
}

/* static */
string DSMSensor::addBackslashSequences(string str)
{
    string res;
    for (unsigned int ic = 0; ic < str.length(); ic++) {
	char c = str[ic];
        switch(c) {
	case '\n':
	    res.append("\\n");
	    break;
	case '\r':
	    res.append("\\r");
	    break;
	case '\t':
	    res.append("\\t");
	    break;
	case '\\':
	    res.append("\\\\");
	    break;
	default:
	    // in C locale isprint returns true for
	    // alpha-numeric, punctuation and the space character
	    // but not other white-space characters like tabs,
	    // newlines, carriage-returns,  form-feeds, etc.
	    if (::isprint(c)) res.push_back(c);
	    else {
		ostringstream ost;
		ost << "\\x" << hex << setw(2) << setfill('0') << (int)(unsigned char) c;
		res.append(ost.str());
	    }
	        
	    break;
	}
    }
    return res;
}

