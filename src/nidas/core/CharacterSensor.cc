/* -*- mode: c++; c-basic-offset: 4; -*-
 ******************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$

 ******************************************************************
*/

#include <nidas/core/CharacterSensor.h>
#include <nidas/core/RTL_IODevice.h>
#include <nidas/core/UnixIODevice.h>

// #include <nidas/util/ThreadSupport.h>

#include <nidas/util/Logger.h>

// #include <asm/ioctls.h>

// #include <cmath>

#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace nidas::core;

namespace n_u = nidas::util;

CharacterSensor::CharacterSensor():
    rtlinux(-1),
    separatorAtEOM(true),
    messageLength(0),
    _promptRate(0.0),
    maxScanfFields(0),
    scanfFailures(0),
    scanfPartials(0),
    _prompted(false)
{
}

CharacterSensor::~CharacterSensor() {
    std::list<AsciiSscanf*>::iterator si;
    for (si = sscanfers.begin(); si != sscanfers.end(); ++si) {
        AsciiSscanf* sscanf = *si;
	delete sscanf;
    }
}

void CharacterSensor::setMessageSeparator(const std::string& val)
    throw(nidas::util::InvalidParameterException)
{
    messageSeparator = n_u::replaceBackslashSequences(val);
    if (getSampleScanner()) getSampleScanner()->setMessageSeparator(messageSeparator);
}

void CharacterSensor::setMessageSeparatorAtEOM(bool val)
    throw(nidas::util::InvalidParameterException)
{
    separatorAtEOM = val;
    if (getSampleScanner()) getSampleScanner()->setMessageSeparatorAtEOM(val);
}

void CharacterSensor::setMessageLength(int val)
    throw(nidas::util::InvalidParameterException)
{
    messageLength = val;
    if (getSampleScanner()) getSampleScanner()->setMessageLength(val);
}

void CharacterSensor::open(int flags)
	throw(n_u::IOException,n_u::InvalidParameterException)
{
    DSMSensor::open(flags);
    // Cannot sendInitString yet.
    // DSMSerialSensors are not yet fully initialized,
    // so it must be done in a derived class open(), i.e.
    // DSMSerialSensor.
}

void CharacterSensor::sendInitString() throw(n_u::IOException)
{
    if (getInitString().length() > 0) {
	DLOG(("sending init string '") << getInitString()
	     << "' to " << getDeviceName());
        string newstr = n_u::replaceBackslashSequences(getInitString());
        write(newstr.c_str(),newstr.length());
    }
}

bool CharacterSensor::isRTLinux() const
{
    if (rtlinux < 0)  {
	const string& dname = getDeviceName();
	string::size_type fs = dname.rfind('/');
	if (fs != string::npos && (fs + 6) < dname.length() &&
	    dname.substr(fs+1,6) == "dsmser")
		    rtlinux = 1;
	else rtlinux = 0;
    }
    return rtlinux == 1;
}
IODevice* CharacterSensor::buildIODevice() throw(n_u::IOException)
{
    if (isRTLinux()) {
	setDriverTimeTagUsecs(USECS_PER_MSEC);
	return new RTL_IODevice();
    }
    else return new UnixIODevice();
}

SampleScanner* CharacterSensor::buildSampleScanner()
{
    SampleScanner* scanr;

    if (isRTLinux()) {
        setDriverTimeTagUsecs(USECS_PER_MSEC);
        scanr = new MessageSampleScanner();
    }
    else {
        MessageStreamScanner* mscanr;
        scanr = mscanr = new MessageStreamScanner();
        mscanr->setNullTerminate(doesAsciiSscanfs());
    }

    scanr->setMessageSeparator(getMessageSeparator());
    scanr->setMessageSeparatorAtEOM(getMessageSeparatorAtEOM());
    scanr->setMessageLength(getMessageLength());
    return scanr;
}

bool CharacterSensor::doesAsciiSscanfs()
{
    for (SampleTagIterator si = getSampleTagIterator(); si.hasNext(); ) {
	const SampleTag* tag = si.next();
	const string& sfmt = tag->getScanfFormat();
	if (sfmt.length() > 0) return true;
    }
    return false;
}

 
void CharacterSensor::init() throw(n_u::InvalidParameterException)
{
    DSMSensor::init();
    for (SampleTagIterator si = getSampleTagIterator(); si.hasNext(); ) {
	const SampleTag* tag = si.next();
	const string& sfmt = tag->getScanfFormat();
	if (sfmt.length() > 0) {
	    AsciiSscanf* sscanf = new AsciiSscanf();
	    try {
	       sscanf->setFormat(n_u::replaceBackslashSequences(sfmt));
	    }
	    catch (n_u::ParseException& pe) {
		throw n_u::InvalidParameterException(getName(),
		       "setScanfFormat",pe.what());
	    }
	    int nv = tag->getVariables().size();
	    sscanf->setSampleTag(tag);
	    sscanfers.push_back(sscanf);
	    if (sscanf->getNumberOfFields() < nv)
		n_u::Logger::getInstance()->log(LOG_WARNING,
		    "%s: number of scanf fields (%d) is less than the number of variables (%d)",
		    getName().c_str(),sscanf->getNumberOfFields(),nv);
	    maxScanfFields = std::max(std::max(maxScanfFields,sscanf->getNumberOfFields()),nv);
	}
	else if (sscanfers.size() > 0) {
	    ostringstream ost;
	    ost << tag->getSampleId();
	    throw n_u::InvalidParameterException(getName(),
	       string("scanfFormat for sample id=") + ost.str(),
	       "Either all samples for a CharacterSensor \
    must have a scanfFormat or no samples");
	}
    }
	
    if (sscanfers.size() > 0) nextSscanfer = sscanfers.begin();
}

void CharacterSensor::fromDOMElement(
	const xercesc::DOMElement* node)
    throw(n_u::InvalidParameterException)
{

    DSMSensor::fromDOMElement(node);

    XDOMElement xnode(node);

    if(node->hasAttributes()) {
    // get all the attributes of the node
	xercesc::DOMNamedNodeMap *pAttributes = node->getAttributes();
	int nSize = pAttributes->getLength();
	for(int i=0;i<nSize;++i) {
	    XDOMAttr attr((xercesc::DOMAttr*) pAttributes->item(i));
	    // get attribute name
	    const std::string& aname = attr.getName();
	    const std::string& aval = attr.getValue();

	    if (aname == "nullterm") { }
	    else if (aname == "init_string")
		setInitString(aval);

	}
    }
    xercesc::DOMNode* child;
    for (child = node->getFirstChild(); child != 0;
	    child=child->getNextSibling())
    {
	if (child->getNodeType() != xercesc::DOMNode::ELEMENT_NODE) continue;
	XDOMElement xchild((xercesc::DOMElement*) child);
	const string& elname = xchild.getNodeName();

	if (elname == "message") {
	    setMessageSeparator(xchild.getAttributeValue("separator"));

	    const string& str = xchild.getAttributeValue("position");
	    if (str == "beg") setMessageSeparatorAtEOM(false);
	    else if (str == "end") setMessageSeparatorAtEOM(true);
	    else if (str != "") throw n_u::InvalidParameterException
			(getName(),"messageSeparator position",str);

	    istringstream ist(xchild.getAttributeValue("length"));
	    int val;
	    ist >> val;
	    if (ist.fail())
		throw n_u::InvalidParameterException(getName(),
		    "message length", xchild.getAttributeValue("length"));
	    setMessageLength(val);
	}
	else if (elname == "prompt") {
	    std::string prompt = xchild.getAttributeValue("string");

	    setPromptString(prompt);

	    istringstream ist(xchild.getAttributeValue("rate"));
	    float rate;
	    ist >> rate;
	    if (ist.fail())
		throw n_u::InvalidParameterException(getName(),
		    "prompt rate", xchild.getAttributeValue("rate"));

	    if (rate < 0.0)
		throw n_u::InvalidParameterException
			(getName(),"prompt rate",
			    xchild.getAttributeValue("rate"));
            setPromptRate(rate);
            //addPrompt(prompt, rate);
	}
    }


    /* determine if any of the samples have associated prompts */
    //list<SampleTag*>::const_iterator si;
    //for (si = getncSampleTags().begin();
             //si != getncSampleTags().end(); ++si) {
        //SampleTag* samp = *si;
        //if (samp->getRate() == 0.0 && getPromptRate() > 0.0)
		//samp->setRate(getPromptRate());
        //if (!samp->getPromptString().empty()) {
		//addPrompt(samp->getPromptString(), samp->getRate());
		//if (samp->getRate() <= 0.0)
		    //throw n_u::InvalidParameterException(
			//getName() + " prompted sensor has sample rate <= 0.0");
        //}
    //}

    //_prompted = !getPrompts().empty();
}

void CharacterSensor::validate() throw(nidas::util::InvalidParameterException)
{
    DSMSensor::validate();

    if (!getPromptString().empty()) addPrompt(getPromptString(), getPromptRate());

    /* determine if any of the samples have associated prompts */
    list<SampleTag*>::const_iterator si;
    for (si = getncSampleTags().begin();
            si != getncSampleTags().end(); ++si) {
	SampleTag* samp = *si;
	if (samp->getRate() == 0.0 && getPromptRate() > 0.0)
	    samp->setRate(getPromptRate());
	if (!samp->getPromptString().empty()) {
	    addPrompt(samp->getPromptString(), samp->getRate());
	    if (samp->getRate() <= 0.0)
	        throw n_u::InvalidParameterException(
		    getName() + " prompted sensor has sample rate <= 0.0");
        }
    }

    _prompted = !getPrompts().empty();
}

bool CharacterSensor::process(const Sample* samp,list<const Sample*>& results)
	throw()
{
    // Note: sscanfers can be empty here, if a CharacterSensor was configured
    // with no samples, and hence no scanf strings.  For example,
    // a differential GPS, where nidas is supposed to take the
    // data for later use, but doesn't (currently) parse it.
    if (sscanfers.size() == 0) return false;

    assert(samp->getType() == CHAR_ST);

    const char* inputstr = (const char*)samp->getConstVoidDataPtr();
    int slen = samp->getDataByteLength();

    // if sample is not null terminated, create a new null-terminated sample
    if (inputstr[slen-1] != '\0') {
        SampleT<char>* newsamp = getSample<char>(slen+1);
        newsamp->setTimeTag(samp->getTimeTag());
        newsamp->setId(samp->getId());
        char* newstr = (char*)newsamp->getConstVoidDataPtr();
        ::memcpy(newstr,inputstr,slen);
        newstr[slen] = '\0';

        bool res =  CharacterSensor::process(newsamp,results);
        newsamp->freeReference();
        return res;
    }

    SampleT<float>* outs = getSample<float>(maxScanfFields);

    const SampleTag* stag = 0;
    int nparsed = 0;
    unsigned int ntry = 0;
    AsciiSscanf* sscanf = 0;
    for ( ; ntry < sscanfers.size(); ntry++) {
	sscanf = *nextSscanfer;
	nparsed = sscanf->sscanf(inputstr,outs->getDataPtr(),
		sscanf->getNumberOfFields());
	if (++nextSscanfer == sscanfers.end()) 
	    nextSscanfer = sscanfers.begin();
	if (nparsed > 0) {
	    stag = sscanf->getSampleTag();
	    outs->setId(stag->getId());
	    if (nparsed != sscanf->getNumberOfFields()) scanfPartials++;
	    break;
	}
    }
    static n_u::LogContext lp(LOG_DEBUG);

    if (lp.active() && nparsed != sscanf->getNumberOfFields())
    {
	n_u::LogMessage msg;
	msg << (nparsed > 0 ? "partial" : "failed")
	    << " scanf; tried " << (ntry+(nparsed>0))
	    << "/" << sscanfers.size() << " formats.\n";
	msg << "input:'" << inputstr << "'\n"
	    << "last format tried: " << (sscanf ? sscanf->getFormat() : "X")
	    << "\n";
	msg << "; nparsed=" << nparsed
	    << "; scanfFailures=" << scanfFailures
	    << "; scanfPartials=" << scanfPartials;
	lp.log(msg);
    }	

    if (!nparsed) {
	scanfFailures++;
	outs->freeReference();	// remember!
	return false;		// no sample
    }

    float* fp = outs->getDataPtr();
    const vector<const Variable*>& vars = stag->getVariables();
    int nv;
    for (nv = 0; nv < (signed)vars.size(); nv++,fp++) {
        const Variable* var = vars[nv];
        if (nv >= nparsed || *fp == var->getMissingValue()) *fp = floatNAN;
        else if (*fp < var->getMinValue() || *fp > var->getMaxValue()) 
            *fp = floatNAN;
        else if (getApplyVariableConversions()) {
            VariableConverter* conv = var->getConverter();
            if (conv) *fp = conv->convert(samp->getTimeTag(),*fp);
        }
    }
    outs->setTimeTag(samp->getTimeTag());
    outs->setDataLength(nv);
    results.push_back(outs);
    return true;
}

