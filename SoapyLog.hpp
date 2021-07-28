// SoapyLog.hpp - common log functions
// Copyright (c) 2021 Phil Ashby
// SPDX-License-Identifier: BSL-1.0

#ifndef SoapyLog_hpp
#define SoapyLog_hpp

#include <SoapySDR/Logger.h>
#include <stdio.h>

/***********************************************************************
 * The following taken from SoapySDR/lib/LoggerC.cpp, oh that this was
 * a public function or that SoapySDR_registerLogHandler() returned the
 * previous pointer.. also SoapySDR_setLogLevel() could have returned
 * the previous level, avoiding ugly detection shanannigans.
 **********************************************************************/

/***********************************************************************
 * ANSI terminal colors for default logger
 **********************************************************************/
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BOLD    "\x1b[1m"

/***********************************************************************
 * Default log message handler implementation
 **********************************************************************/
static void defaultLogHandler(const SoapySDRLogLevel logLevel, const char *message)
{
    switch (logLevel)
    {
    case SOAPY_SDR_FATAL:    fprintf(stderr, ANSI_COLOR_BOLD ANSI_COLOR_RED "[FATAL] %s" ANSI_COLOR_RESET "\n", message); break;
    case SOAPY_SDR_CRITICAL: fprintf(stderr, ANSI_COLOR_BOLD ANSI_COLOR_RED "[CRITICAL] %s" ANSI_COLOR_RESET "\n", message); break;
    case SOAPY_SDR_ERROR:    fprintf(stderr, ANSI_COLOR_BOLD ANSI_COLOR_RED "[ERROR] %s" ANSI_COLOR_RESET "\n", message); break;
    case SOAPY_SDR_WARNING:  fprintf(stderr, ANSI_COLOR_BOLD ANSI_COLOR_YELLOW "[WARNING] %s" ANSI_COLOR_RESET "\n", message); break;
    case SOAPY_SDR_NOTICE:   fprintf(stderr, ANSI_COLOR_GREEN "[NOTICE] %s" ANSI_COLOR_RESET "\n", message); break;
    case SOAPY_SDR_INFO:     fprintf(stderr, ANSI_COLOR_CYAN "[INFO] %s" ANSI_COLOR_RESET "\n", message); break;
    case SOAPY_SDR_DEBUG:    fprintf(stderr, ANSI_COLOR_BLUE "[DEBUG] %s" ANSI_COLOR_RESET "\n", message); break;
    case SOAPY_SDR_TRACE:    fprintf(stderr, "[TRACE] %s\n", message); break;
    case SOAPY_SDR_SSI:      fputs(message, stderr); fflush(stderr); break;
    }
}

/***********************************************************************
 * Log level detection mechanism (see above!)
 **********************************************************************/
static bool tempLogged;
static void tempLogHandler(SoapySDRLogLevel lev, const char *msg)
{
    tempLogged = true;
}

static inline SoapySDRLogLevel detectLogLevel()
{
    SoapySDRLogLevel found=SOAPY_SDR_FATAL;
    SoapySDR_registerLogHandler(tempLogHandler);
    for(int lev=(int)SOAPY_SDR_FATAL; lev<(int)SOAPY_SDR_SSI; ++lev) {
        tempLogged=false;
        SoapySDR_log((SoapySDRLogLevel)lev, "detect..");
        if (tempLogged)
            found=(SoapySDRLogLevel)lev;
    }
    SoapySDR_registerLogHandler(defaultLogHandler);
    return found;
}

#endif