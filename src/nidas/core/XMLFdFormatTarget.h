
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_CORE_XMLFDFORMATTARGET_H
#define NIDAS_CORE_XMLFDFORMATTARGET_H

#include <xercesc/framework/XMLFormatter.hpp>
#include <xercesc/util/XercesDefs.hpp>

#include <nidas/util/IOException.h>

#include <string>

namespace nidas { namespace core {

/**
 * Extension of xercesc::XMLFormatTarget support writing
 * XML to an open device (socket for example).
 */
class XMLFdFormatTarget : public xercesc::XMLFormatTarget {
public:

    /**
     * Constructor.
     * @param n name of device - only used when reporting errors.
     * @param f unix file descriptor of device that is already open.
     */
    XMLFdFormatTarget(const std::string& n, int f);

    /**
     * Destructor.  Does not close file descriptor.
     */
    ~XMLFdFormatTarget();
                                                                                
    void flush() throw(nidas::util::IOException);
                                                                                
    /**
     * Implemention of virtual write method of xercesc::XMLFormatTarget.
     * Does buffered writes to the file descriptor.
     */
    void writeChars(const XMLByte*const toWrite, 
    	const unsigned int count,
        xercesc::XMLFormatter *const ) throw(nidas::util::IOException);

private:
    void insureCapacity(unsigned int count) throw(nidas::util::IOException);

    std::string name;
    int fd;
    XMLByte* fDataBuf;
    unsigned int    fIndex;
    unsigned int    fCapacity;
};

}}	// namespace nidas namespace core

#endif
                                                                                
