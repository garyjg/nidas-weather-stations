// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4; -*-
// vim: set shiftwidth=4 softtabstop=4 expandtab:
/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_CORE_CALFILE_H
#define NIDAS_CORE_CALFILE_H

#include <nidas/core/DOMable.h>
#include <nidas/util/UTime.h>
#include <nidas/util/IOException.h>
#include <nidas/util/EOFException.h>

#include <vector>
#include <fstream>

#include <regex.h>

namespace nidas { namespace core {

class DSMSensor;

/**
 * A class for reading ASCII files containing a time series of
 * calibration data.
 *
 * CalFile supports reading files like the following:
 *
 * <pre>
 *  # example cal file
 *  # dateFormat = "%Y %b %d %H:%M:%S"
 *  # timeZone = "US/Mountain"
 *  #
 *  2006 Sep 23 00:00:00    0.0 1.0
 *  # Offset of 1.0 after Sep 29 01:13:00
 *  2006 Sep 29 01:13:00    1.0 1.0
 *  # use calibrations for ACME sensor SN#99 after Oct 1
 *  2006 Oct 01 00:00:00    include "acme_sn99.dat"
 * </pre>
 *
 * As shown, comment lines begin with a '#'.  There are two
 * special comment lines, of the form 'dateFormat="blahblah"'
 * and 'timeZone="blahblah".  These specify the format
 * of time in the calibration file.  The dateFormat
 * should contain date and time format descriptors as 
 * supported by the UNIX strftime function or as 
 * supported by the java.text.SimpleDateFormat class.
 * Typically the dateFormat comment is found once
 * in the file, before any data records.
 * 
 * If the dateFormat comment is not found in a file, and the
 * dateTimeFormat attribute is not set on the instance of
 * CalFile, then times must be in a format supported by
 * nidas::util::UTime::parse.
 *
 * <table>
 *  <tr>
 *    <th>field<th>example<th>UNIX<th>java
 *  <tr>
 *    <td>year<td>2006<td>\%Y<td>YYYY
 *  <tr>
 *    <td>month abrev<td>Sep<td>\%b<td>MMM
 *  <tr>
 *    <td>numeric month<td>9<td>\%m<td>MM
 *  <tr>
 *    <td>day of month<td>14<td>\%d<td>dd
 *  <tr>
 *    <td>day of year (1-366)<td>257<td>\%j<td>DDD
 *  <tr>
 *    <td>hour in day (00-23)<td>13<td>\%H<td>HH
 *  <tr>
 *    <td>minute(00-59)<td>24<td>\%M<td>mm
 *  <tr>
 *    <td>second(00-59)<td>47<td>\%S<td>ss
 *  <tr>
 *    <td>millisecond(000-999)<td>447<td>\%3f<br>(UTime extension)<td>SSS
 *  </table>
 * 
 *  Following the time fields in each record should be either
 *  numeric values or an "include" directive.
 *
 *  The numeric values should be space or tab separated
 *  values compatible with the usual floating point syntax,
 *  or the strings "na" or "nan" in either upper or lower case,
 *  representing not-a-number, or NaN.
 *  Since math operations using NaN result in a NaN, a
 *  calibration record containing a NaN value is a way to
 *  generate output values of NaN, indicating non-recoverable
 *  data.
 *
 *  An "include" directive causes another calibration
 *  file to be opened for input.  The included file will
 *  be sequentially searched to set the input position to the
 *  last record with a time less than or equal to the time
 *  value of the "include directive. What this means
 *  is that the next readData() will return data
 *  from the included file which is valid for the
 *  time of the include directive.
 *
 *  An included file can also contain "include" directives.
 *
 *  The include directive is useful when sensors are swapped
 *  during a data acquisition period.  One can keep the
 *  sensor specific calibrations in separate files, and
 *  then create a CalFile which includes the sensor
 *  calibrations for the periods that a sensor was deployed.
 *  Example:
 * <pre>
 *  # initially krypton hygrometer 1101 at this site
 *  2006 Sep 23 00:00:00    include "krypton1101"
 *  # Replaced 1101 with 1393 on Oct 3.
 *  2006 Oct  3 01:13:00    include "krypton1393"
 * </pre>
 *
 *  A typical usage of a CalFile is as follows:
 *
 * <pre>
 *  CalFile calfile;
 *  calfile.setFile("acme_sn1.dat")
 *  calfile.setPath("$ROOT/projects/$PROJECT/cal_files:$ROOT/cal_files");
 *  dsm_time_t calTime = 0;
 *
 *  ...
 *  while (tsample > calTime) {
 *      try {
     *      float caldata[5];
 *          int n = calfile.readData(caldata,5);
 *          for (int i = 0; i < n; i++) coefs[i] = caldata[i];
 *          // read the time of the next calibration record
 *          calTime = calfile.readTime();
 *      }
 *      catch(const nidas::util::IOException& e) {
 *          log(e.what);
 *      }
 *      catch(const nidas::util::ParseException& e) {
 *          log(e.what);
 *      }
 *  }
 *  // use coefs[] to calibrate sample.
 * </pre>
 *      
 */
class CalFile: public nidas::core::DOMable {
public:

    CalFile();

    /**
     * Copy constructor. Copies the value of getFileName() and
     * getPath() attributes.  The CalFile will not be
     * opened in the new copy.
     */
    CalFile(const CalFile&);

    /**
     * Assignment operator, like the copy constructor.
     * If a file is currently open it will be closed
     * before the assignment.
     */
    CalFile& operator=(const CalFile&);

    /**
     * Closes the file if necessary.
     */
    ~CalFile();

    const std::string& getFile() const;

    /**
     * Set the base name of the file to be opened.
     */
    void setFile(const std::string& val);

    /**
     * Set the search path to find the file, and any
     * included files: one or more directory paths
     * separated by colons ':'.
     */
    const std::string& getPath() const;

    /**
     * Set the search path to find the file, and any
     * included files: one or more directory paths
     * separated by colons ':'.
     */
    void setPath(const std::string& val);

    /**
     * Return all the paths that have been set in all CalFile instances,
     * in the order they were seen.  These have been separated at the colons.
     */
    static std::vector<std::string> getAllPaths()
    {
        _staticMutex.lock();
        std::vector<std::string> tmp = _allPaths;
        _staticMutex.unlock();
        return tmp;
    }

    /**
     * Return the full file path of the current file.
     */
    const std::string& getCurrentFileName() const
    {
        if (_include) return _include->getCurrentFileName();
        return _currentFileName;
    }

    int getLineNumber() const
    {
        if (_include) return _include->getLineNumber();
        return _nline;
    }

    /** 
     * Open the file. It is not necessary to call open().
     * If the user has not done an open() it will
     * be done in the first readTime, readData, or search().
     */
    void open() throw(nidas::util::IOException);

    /**
     * Close file. An opened CalFile is closed in the destructor,
     * so it is not necessary to call close.
     */
    void close();

    /**
     * Have we reached eof.
     */
    bool eof() const {
        if (_include) return false;
        return _eofState;
    }

    const std::string& getTimeZone() const { return _timeZone; }

    void setTimeZone(const std::string& val)
    {
        _timeZone = val;
        _utcZone = _timeZone == "GMT" || _timeZone == "UTC";
    }

    const std::string& getDateTimeFormat() const
    {
        return _dateTimeFormat;
    }

    /**
     * Set the format for reading the date & time from the file.
     * If a "dateFormat" comment is found at the beginning of the
     * file, this attribute will be set to that value.
     */
    void setDateTimeFormat(const std::string& val);

    /** 
     * Search forward in a file, returning the time of the last record
     * in the file with a time less than or equal to tsearch.
     * The next call to readData() will read the data portion of that 
     * record.
     */
    nidas::util::UTime  search(const nidas::util::UTime& tsearch)
        throw(nidas::util::IOException,nidas::util::ParseException);

    /** 
     * Read the time from the next record. If the EOF is found
     * the returned time will be a huge value, far off in the
     * mega-distant future. Does not return an EOFException
     * on EOF.
     */
    nidas::util::UTime readTime()
        throw(nidas::util::IOException,nidas::util::ParseException);

    /**
     * Read the data from the current record. The return
     * value may be less than ndata, in which case
     * values in data after n will be filled with NANs.
     * This can return an EOF.
     */
    int readData(float* data, int ndata)
        throw(nidas::util::IOException,nidas::util::ParseException);

    /**
     * Set the DSMSensor associated with this CalFile.
     * CalFile may need this in order to substitute
     * for tokens like $DSM and $HEIGHT in the file or path names.
     * Otherwise it is not necessary to setDSMSensor.
     */
    void setDSMSensor(const DSMSensor* val);

    const DSMSensor* getDSMSensor() const;

    void fromDOMElement(const xercesc::DOMElement* node)
	throw(nidas::util::InvalidParameterException);

protected:

    nidas::util::UTime parseTime() throw(nidas::util::ParseException);

    void readLine() throw(nidas::util::IOException,nidas::util::ParseException);

    void openInclude(const std::string& name)
        throw(nidas::util::IOException,nidas::util::ParseException);

private:

    std::string _fileName;

    std::string _path;

    std::string _currentFileName;

    std::string _timeZone;

    bool _utcZone;

    std::string _dateTimeFormat;

    std::ifstream _fin;

    static const int INITIAL_CURLINE_LENGTH = 128;

    int _curlineLength;

    char *_curline;

    int _curpos;

    bool _eofState;

    int _nline;

    nidas::util::UTime _curTime;

    /**
     * Time stamp of include "file" record.
     */
    nidas::util::UTime _includeTime;

    /**
     * Time stamp of record after include "file".
     */
    nidas::util::UTime _timeAfterInclude;

    /**
     * Time stamp of last record in include file with time <= _includeTime.
     */
    nidas::util::UTime _timeFromInclude;

    CalFile* _include;

    const DSMSensor* _sensor;

    static nidas::util::Mutex _staticMutex;

    static int _reUsers;

    static bool _reCompiled;

    static regex_t _dateFormatPreg;

    static regex_t _timeZonePreg;

    static regex_t _includePreg;

    static void freeREs();

    static void compileREs() throw(nidas::util::ParseException);

    static std::vector<std::string> _allPaths;
};

}}	// namespace nidas namespace core

#endif
