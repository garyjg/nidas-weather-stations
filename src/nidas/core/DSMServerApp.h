/*
 ********************************************************************
    Copyright 2005 UCAR, NCAR, All Rights Reserved

    $LastChangedDate: 2009-03-26 22:35:58 -0600 (Thu, 26 Mar 2009) $

    $LastChangedRevision: 4548 $

    $LastChangedBy: maclean $

    $HeadURL: http://svn.eol.ucar.edu/svn/nidas/trunk/src/nidas/core/DSMServer.h $
 ********************************************************************

*/

#ifndef NIDAS_CORE_DSMSERVERAPP_H
#define NIDAS_CORE_DSMSERVERAPP_H

#include <nidas/core/DSMServer.h>
#include <nidas/core/XMLParser.h>
#include <nidas/core/DSMServerIntf.h>
#include <nidas/core/ProjectConfigs.h>

namespace nidas { namespace core {

class DSMServerApp {
public:

    DSMServerApp();

    /**
    * Run DSMServerApp from the unix command line.
    */
    static int main(int argc, char** argv) throw();

    static DSMServerApp* getInstance() { return _instance; }

    static void setupSignals();

    static void unsetupSignals();

    void initLogger();

    int run() throw();

    int parseRunstring(int argc, char** argv);

    /**
     * Send usage message to cerr.
     */
    int usage(const char* argv0);

    Project* parseXMLConfigFile(const std::string& xmlFileName)
        throw(nidas::core::XMLException,
            nidas::util::InvalidParameterException,nidas::util::IOException);

    void interruptQuit() throw();

    void interruptRestart() throw();

    void startXmlRpcThread() throw(nidas::util::Exception);

    void killXmlRpcThread() throw();

    void startStatusThread(DSMServer* svr) throw(nidas::util::Exception);

    void killStatusThread() throw();


    /**
     * What is the XML configuration file name.
     */
    const std::string& getXMLFileName() { return _xmlFileName; }

    std::string getUserName()
    { 
        return _username;
    }

    uid_t getUserID()
    {
        return _userid;
    }

    uid_t getGroupID()
    {
        return _groupid;
    }

private:

    static void sigAction(int sig, siginfo_t* siginfo, void* vptr);

    static DSMServerApp* _instance;

    enum runState { RUN, QUIT, RESTART };

    /**
     * -d option. If user wants messages on stderr rather than syslog.
     */
    bool _debug;

    /**
     * The xml file that is being used for configuration information.
     */
    std::string _xmlFileName;

    /**
     * The xml file that contains all the project configurations, by date.
     */
    std::string _configsXMLName;

    const char* _rafXML;

    const char* _isffXML;

    nidas::util::Cond _runCond;
    
    enum runState _runState;

    std::string _username;

    uid_t _userid;

    gid_t _groupid;

    /** This thread provides XML-based Remote Procedure calls */
    DSMServerIntf* _xmlrpcThread;

    DSMServerStat* _statusThread;

};

}}	// namespace nidas namespace core

#endif
