// SoapyRPC.hpp - remote procedure call endpoint
// Copyright (c) 2021 Phil Ashby
// License-SPDX ...

#ifndef SoapyRPC_hpp
#define SoapyRPC_hpp

// This RPC implementation uses text I/O over TCP
// in the tradition of many 'simple xxx' internet
// protocols.

#include <SoapySDR/Logger.hpp>
#include <stdio.h>
#include <string.h>

// map of format names to frame sizes
const std::map<std::string, size_t> g_frameSizes = {
    { "CS8", 2 }, { "CS16", 4 }, { "CF32", 8 },
};

// RPC codes
enum
{
    // internal specials - socket type identifier, single digit value please..
    TCPREMOTE_RPC_LOAD,
    TCPREMOTE_DATA_SEND,
    TCPREMOTE_DATA_RECV,
    // identification API
    TCPREMOTE_GET_HARDWARE_KEY,
    TCPREMOTE_GET_HARDWARE_INFO,
    // channel API
    TCPREMOTE_GET_NUM_CHANNELS,
    TCPREMOTE_GET_CHANNEL_INFO,
    // stream API
    TCPREMOTE_GET_STREAM_FORMATS,
    TCPREMOTE_GET_STREAM_NATIVE_FORMAT,
    TCPREMOTE_GET_STREAM_ARGS_INFO,
    TCPREMOTE_SETUP_STREAM,
    TCPREMOTE_CLOSE_STREAM,
    TCPREMOTE_GET_STREAM_MTU,
    TCPREMOTE_ACTIVATE_STREAM,
    TCPREMOTE_DEACTIVATE_STREAM,
#if 0
    //All the other remote methods..
#endif
};

class SoapyRPC
{
public:
    SoapyRPC(int socket) {
        handle = fdopen(socket, "r+");
        if (handle)
            setlinebuf(handle);
        else
            SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyRPC::<cons>, failed to fdopen socket: %s", strerror(errno));
    }
    SoapyRPC(FILE *fp) {
        handle = fp;
        setlinebuf(handle);
    }
    ~SoapyRPC() {
        if (handle)
            fclose(handle);
    }
    int writeInteger(const int i) {
        return fprintf(handle, "%d\n", i);
    }
    int writeString(const std::string s) {
        return fprintf(handle, "%s\n", s.c_str());
    }
    int writeKwargs(const SoapySDR::Kwargs &args) {
        int n = 0;
        for (auto it=args.begin(); it!=args.end(); ++it) {
            std::string nv;
            nv += it->first;
            nv += '=';
            nv += it->second;
            n += writeString(nv);
        }
        n += writeString("=");      // terminator
        return n;
    }
    int readInteger() {
        int rv=-1;
        std::string s = readString();
        if (s.length()>0)
            sscanf(s.c_str(), "%d", &rv);
        else
            SoapySDR_log(SOAPY_SDR_ERROR, "SoapyRPC::readInteger, empty line");
        return rv;
    }
    std::string readString() {
        char line[256];
        std::string rv;
        if (fgets(line, sizeof(line), handle)) {
            line[strlen(line)-1] = 0;
            rv = line;
        } else
            SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyRPC::readString, failed to read data: %s", strerror(errno));
        return rv;
    }
    SoapySDR::Kwargs readKwargs() {
        SoapySDR::Kwargs args;
        while (true) {
            std::string nv = readString();
            if (nv.length()<2)      // '=' or empty is a terminator
                break;
            size_t p = nv.find('=');
            if (p>0) {
                args[nv.substr(0,p)] = nv.substr(p+1);
            } else {
                SoapySDR_logf(SOAPY_SDR_INFO,"SoapyRPC::readKwargs, ignored invalid line: %s", nv.c_str());
            }
        }
        return args;
    }

private:
    FILE *handle;
};

#endif
