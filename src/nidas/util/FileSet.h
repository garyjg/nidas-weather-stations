/* -*- mode: c++; c-basic-offset: 4; -*-
 ********************************************************************
    Copyright by the National Center for Atmospheric Research

    $LastChangedDate$

    $LastChangedRevision$

    $LastChangedBy$

    $HeadURL$
 ********************************************************************

*/

#ifndef NIDAS_UTIL_FILESET_H
#define NIDAS_UTIL_FILESET_H

#include <nidas/util/IOException.h>
#include <nidas/util/UTime.h>

#include <list>
#include <set>
#include <string>
#include <locale>
#include <ctime>
#include <limits.h>
#include <sys/uio.h>

namespace nidas { namespace util {

/**
 * A description of a set of output files, consisting of a 
 * directory name and a file name format containing UNIX strftime
 * conversion specifiers, like %Y, %m, etc. Typically this class
 * is used by an archive method.
 */
class FileSet {
public:

    /**
     * constructor
     */
    FileSet();

    /**
     * Copy constructor. Only permissable before it is opened.
     */
    FileSet(const FileSet& x);

    /**
     * Destructor. Closes current file, ignoring possible IOException
     * if the file isn't open.
     */
    virtual ~FileSet();

    /**
     * Set directory portion of file path.  This may
     * contain strftime conversion specifiers, like %Y in order to
     * put a year in the directory name.
     */
    virtual void setDir(const std::string& val);

    virtual const std::string& getDir() { return dir; }

    const int getFd() const { return fd; }

    bool isNewFile() const { return newFile; }

    /**
     * Set file name portion of file path. This can contain
     * strftime conversion specifiers.  FileSet inserts
     * the pathSeparator between the directory and the file name
     * when creating the full file path string. Otherwise there
     * is no distinction between the directory and file portion.
     */
    virtual void setFileName(const std::string& val);

    virtual const std::string& getFileName() { return filename; }

    virtual void addFileName(const std::string& val) { fileset.push_back(val); }

    /**
     * Set/get the file length in seconds.
     */
    void setFileLengthSecs(int val)
    {
	if (val <= 0) fileLength = 400 * USECS_PER_DAY;
	else fileLength = (long long) val * USECS_PER_SEC;
    }

    int getFileLengthSecs() const
    {
	if (fileLength > 365 * USECS_PER_DAY) return 0;
        return (int)(fileLength / USECS_PER_SEC);
    }

    UTime getNextFileTime() const
    {
	return nextFileTime;
    }

    /**
     * Create a new file, with a name formed from a time.
     * @param tfile Time to use when creating file name.
     * @param exact Use exact time when creating file name, else
     *        the time is truncated by getFileLengthSecs.
     * @return Start time of next file, i.e. when to create next file.
     */
    UTime createFile(UTime tfile,bool exact) throw(IOException);

    void setStartTime(const UTime& val) { startTime = val; } 

    UTime getStartTime() const { return startTime; } 

    void setEndTime(const UTime& val) { endTime = val; } 

    UTime getEndTime() const { return endTime; } 

    /**
     * Get name of current file.
     */
    const std::string& getCurrentName() const { return currname; }

    /**
     * Closes any file currently open.  The base implementation
     * closes the file descriptor.  Subclasses override this method
     * to close alternate resources.
     **/
    virtual void
    closeFile() throw(IOException);

    /**
     * Open a new file for writing.  The @p filename is the path for the
     * new file as generated from the filename template and a time.  The
     * base implementation calls open64() and sets the file descriptor.
     **/
    virtual void
    openFileForWriting(const std::string& filename) throw(IOException);

    /**
     * Open the next file to be read.
     **/
    void openNextFile() throw(IOException);

    /**
     * Read from current file.
     */
    size_t read(void* buf, size_t count) throw(IOException);

    /**
     * Write to current file.
     */
    size_t write(const void* buf, size_t count) throw(IOException);

    size_t write(const struct iovec* iov, int iovcnt) throw(IOException);

    static const char pathSeparator;

    static void createDirectory(const std::string& name)
        throw(IOException);

    /**
     * Utility function to return the directory portion of a
     * file path. If the directory portion is empty, returns ".".
     * Uses pathSeparator to determine directory and file portion.
     */
    static std::string getDirPortion(const std::string& path);

    /**
     * Utility function to return the file portion of a
     * file path.  Uses pathSeparator to determine directory and file
     * portion.
     */
    static std::string getFilePortion(const std::string& path);

    /**
     * Utility function to create a full path name from a directory
     * and file portion. If the directory portion is an empty string,
     * or ".", then the result path will have no directory portion.
     */
    static std::string makePath(const std::string& dir,const std::string& file);

#if !defined(NIDAS_EMBEDDED)
    /**
     * Check that any date or time descriptors, e.g. "%y", "%m", in
     * the full file path string are in the correct order, so that
     * a default lexical sort will sort the file path names in time order.
     * The descriptors in the path must be in decreasing time-interval
     * order, e.g. year before month, month before day, etc.
     */
    void checkPathFormat(const UTime& t1, const UTime& t2) throw(IOException);
#endif

    std::list<std::string> matchFiles(const UTime& t1, const UTime& t2)
    	throw(IOException);

    long long getFileSize() const throw(IOException);

    /**
     * Get last error value. Should be 0. Currently only supported
     * for an output file, to be queried by a system status thread.
     */
    int getLastErrno() const 
    {
        return _lastErrno;
    }

protected:

    std::string formatName(const UTime& t1);

    static void replaceChars(std::string& in,const std::string& pat,
    	const std::string& rep);

    const std::time_put<char> &timeputter;

private:
    std::string dir;

    std::string filename;

    std::string currname;

    std::string fullpath;

    int fd;

    UTime startTime;
    UTime endTime;

    std::list<std::string> fileset;
    std::list<std::string>::iterator fileiter;

    bool initialized;

    /**
     * File length, in microseconds.
     */
    long long fileLength;

    UTime nextFileTime;

    bool newFile;

    int _lastErrno;

};

}}	// namespace nidas namespace util

#endif
