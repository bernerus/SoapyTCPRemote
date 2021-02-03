//  SoapyTCPRemote.cpp
//  Copyright (c) 2021 Phil Ashby
//  SPDX-License-Identifier: BSL-1.0

#include "SoapyTCPRemote.hpp"

#include <SoapySDR/Logger.hpp>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

// declare the contents of a Stream object for ourselves
class SoapySDR::Stream
{
public:
    FILE* dataFp;
    int remoteId;
    int numChans;
    size_t fSize;
    bool running;
};

SoapyTCPRemote::SoapyTCPRemote(const std::string &address, const std::string &port, const std::string &remdriver, const std::string &remargs) :
    remoteAddress(address),
    remotePort(port),
    remoteDriver(remdriver),
    remoteArgs(remargs)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::<cons>(%s,%s,%s,%s)",
        address.c_str(), port.c_str(), remdriver.c_str(), remargs.c_str());
    int sock = connect();
    if (sock<0)
        throw std::runtime_error("unable to connect to remote");
    rpc = new SoapyRPC(sock);
    int status = loadRemoteDriver();
    if (status<0)
        throw std::runtime_error("unable to load remote driver");
}

SoapyTCPRemote::~SoapyTCPRemote()
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::<dest>");
    if (rpc) {
        delete rpc;
        rpc = nullptr;
    }
}

// private connector method
int SoapyTCPRemote::connect() const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::connect()");
    // create new socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Unable to create socket: %s", strerror(errno));
        return -1;
    }
    // resolve address (or parse)
    struct addrinfo *res = nullptr;
    if (getaddrinfo(remoteAddress.c_str(), remotePort.c_str(), nullptr, &res)) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Failed to resolve address/port: %s/%s: %s",
            remoteAddress.c_str(), remotePort.c_str(), strerror(errno));
        close(sock);
        return -1;
    }
    // attempt connection to remote server
    if (::connect(sock, res->ai_addr, res->ai_addrlen)) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Failed to connect to address/port: %s/%s: %s",
            remoteAddress.c_str(), remotePort.c_str(), strerror(errno));
        freeaddrinfo(res);
        close(sock);
        return -1;
    }
    freeaddrinfo(res);
    SoapySDR_logf(SOAPY_SDR_DEBUG, "SoapyTCPRemote: connected: %s/%s",
            remoteAddress.c_str(), remotePort.c_str());
    return sock;
}

int SoapyTCPRemote::loadRemoteDriver() const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::loadRemoteDriver()");
    // identify this connection as an RPC stream load the remote driver
    int status = rpc->writeInteger(TCPREMOTE_RPC_LOAD);
    if (status>0)
        status = rpc->writeString(remoteDriver);
    if (status>0)
        status = rpc->writeString(remoteArgs);
    if (status>0)
        status = rpc->readInteger();
    if (status<0) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "Failed to load remote driver: %s(%s)=%d",
            remoteDriver.c_str(), remoteArgs.c_str(), status);
    }
    return status;
}

// Identification API
std::string SoapyTCPRemote::getHardwareKey() const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getHardwareKey()");
    rpc->writeInteger(TCPREMOTE_GET_HARDWARE_KEY);
    return rpc->readString();
}

SoapySDR::Kwargs SoapyTCPRemote::getHardwareInfo() const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getHardwareInfo()");
    rpc->writeInteger(TCPREMOTE_GET_HARDWARE_INFO);
    return rpc->readKwargs();
}

// Channels API
size_t SoapyTCPRemote::getNumChannels(const int dir) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getNumChannels()");
    int status = rpc->writeInteger(TCPREMOTE_GET_NUM_CHANNELS);
    if (status>0)
        status = rpc->writeInteger(dir);
    if (status>0)
        status = rpc->readInteger();
    return status;
}

SoapySDR::Kwargs SoapyTCPRemote::getChannelInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getChannelInfo()");
    int status = rpc->writeInteger(TCPREMOTE_GET_CHANNEL_INFO);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return rpc->readKwargs();
}

bool SoapyTCPRemote::getFullDuplex(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFullDuplex()");
    int status = rpc->writeInteger(TCPREMOTE_GET_FULL_DUPLEX);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return rpc->readInteger()!=0;
}

std::vector<std::string> SoapyTCPRemote::getStreamFormats(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getStreamFormats()");
    int status = rpc->writeInteger(TCPREMOTE_GET_STREAM_FORMATS);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return rpc->readStrVector();
}

std::string SoapyTCPRemote::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getNativeStreamFormat()");
    int status = rpc->writeInteger(TCPREMOTE_GET_STREAM_NATIVE_FORMAT);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    std::string fmt;
    if (status>0)
        fmt = rpc->readString();
    if (fmt.length()>0)
        status = rpc->readInteger(); // assumes integer full scale??
    if (status>0)
        fullScale = (double)status;
    return fmt;
}

SoapySDR::ArgInfoList SoapyTCPRemote::getStreamArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getStreamArgsInfo()");
    int status = rpc->writeInteger(TCPREMOTE_GET_STREAM_ARGS_INFO);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    SoapySDR::ArgInfoList info;
    while (status>0) {
        // fugly complex structure to serialise... maybe later!
        std::string s = rpc->readString();
        if (s.length()==0)
            break;
    }
    SoapySDR_log(SOAPY_SDR_ERROR, "Unimplemented: getStreamArgsInfo");
    return info;
}

SoapySDR::Stream *SoapyTCPRemote::setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels, const SoapySDR::Kwargs &args)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::setupStream(%d,%s,%d,...)",
        direction, format.c_str(), channels.size());
    // first check we have a frame size for the format
    if (g_frameSizes.find(format)==g_frameSizes.end()) {
        SoapySDR_log(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, unknown sample format");
        return nullptr;
    }
    // in order to help the remote side associate the data stream with the setup call,
    // we create the data connection *first*, then send it's remoteId as the first
    // parameter to the RPC call..
    int data = connect();
    if (data<0) {
        SoapySDR_log(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, data stream failed to connect");
        return nullptr;
    }
    // sending one of TCPREMOTE_DATA_<x> makes this a data stream in the remote
    char dir[10];
    int dlen = sprintf(dir, "%d\n", SOAPY_SDR_RX==direction? TCPREMOTE_DATA_SEND : TCPREMOTE_DATA_RECV);
    if (write(data, dir, dlen)!=dlen) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, failed to write data stream type: %s",
            strerror(errno));
        close(data);
        return nullptr;
    }
    dlen = read(data, dir, sizeof(dir));
    if (dlen<=0) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, failed to read data stream remoteId: %s",
            strerror(errno));
        close(data);
        return nullptr;
    }
    SoapySDR::Stream *rv = new SoapySDR::Stream();
    dir[dlen]=0;
    sscanf(dir, "%d", &rv->remoteId);
    rv->dataFp = fdopen(data, SOAPY_SDR_RX==direction? "r": "w");
    rv->fSize = g_frameSizes.at(format);
    rv->numChans = channels.size();
    rv->running = false;
    // make the RPC call with the remoteId
    int status = rpc->writeInteger(TCPREMOTE_SETUP_STREAM);
    if (status>0)
        status = rpc->writeInteger(rv->remoteId);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeString(format);
    if (status>0) {
        // channel list, sent as a space separated list of numbers on one line
        std::string chans;
        for (auto it=channels.begin(); it!=channels.end(); ++it) {
            if (chans.length()>0)
                chans += " ";
            chans += std::to_string(*it);
        }
        status = rpc->writeString(chans);
    }
    if (status>0)
        status = rpc->writeKwargs(args);
    if (status>0)
        status = rpc->readInteger();
    if (status>=0) {
        SoapySDR_logf(SOAPY_SDR_TRACE,"SoapyTCPRemote::setupStream, data stream remoteId: %d", rv->remoteId);
    } else {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, error: %d", status);
        if (rv)
            closeStream(rv);
        return nullptr;
    }
    return rv;
}

void SoapyTCPRemote::closeStream(SoapySDR::Stream *stream)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::closeStream()");
    if (stream->running)
        deactivateStream(stream);
    rpc->writeInteger(TCPREMOTE_CLOSE_STREAM);
    fclose(stream->dataFp);
    delete stream;
}

size_t SoapyTCPRemote::getStreamMTU(SoapySDR::Stream *stream) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getStreamMTU()");
    int status = rpc->writeInteger(TCPREMOTE_GET_STREAM_MTU);
    if (status>0)
        status = rpc->writeInteger(stream->remoteId);
    return rpc->readInteger();
}

int SoapyTCPRemote::activateStream(SoapySDR::Stream *stream,
                               const int flags,
                               const long long timeNs,
                               const size_t numElems)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::activateStream()");
    if (stream->running)
        return 0;
    int status = rpc->writeInteger(TCPREMOTE_ACTIVATE_STREAM);
    if (status>0)
        status = rpc->writeInteger(stream->remoteId);
    if (status>0)
        status = rpc->readInteger();
    if (status>0)
        stream->running = true;
    return status;
}

int SoapyTCPRemote::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::deactivateStream()");
    if (!stream->running)
        return 0;
    int status = rpc->writeInteger(TCPREMOTE_DEACTIVATE_STREAM);
    if (status>0)
        status = rpc->writeInteger(stream->remoteId);
    if (status>0)
        status = rpc->readInteger();
    if (status>0)
        stream->running = false;
    return status;
}

int SoapyTCPRemote::readStream(SoapySDR::Stream *stream,
                           void * const *buffs,
                           const size_t numElems,
                           int &flags,
                           long long &timeNs,
                           const long timeoutUs)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::readStream()");
    // Not running? timeout (says the docs)
    if (!stream->running)
        return SOAPY_SDR_TIMEOUT;
    // Transfer format on the wire is interleaved sample frames (each fSize) across channels.
    // We read by making one syscall for the maximum amount, then de-interleaving to buffs.
    // NB: use of fread & stdio to ensure block sizes are reliable despite TCP/IP underneath!
    size_t blkSize = stream->fSize * stream->numChans;
    uint8_t *swamp = (uint8_t *)alloca(blkSize * numElems);
    int status = fread(swamp, blkSize, numElems, stream->dataFp);
    if (status<=0) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::readStream, error reading data: %s", strerror(errno));
        return SOAPY_SDR_STREAM_ERROR;
    }
    int elems=0;
    int soff=0;
    int boff=0;
    while (elems<status) {
        for (int c=0; c<stream->numChans; ++c) {
            uint8_t *buf = (uint8_t *)(buffs[c]);
            memcpy(buf+boff, swamp+soff, stream->fSize);
            soff += stream->fSize;
        }
        boff += stream->fSize;
        ++elems;
    }
    return elems;
}

int SoapyTCPRemote::writeStream(SoapySDR::Stream *stream,
                    const void * const *buffs,
                    const size_t numElems,
                    int &flags,
                    const long long timeNs,
                    const long timeoutUs)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::writeStream()");
    // Not running? timeout (says the docs)
    if (!stream->running)
        return SOAPY_SDR_TIMEOUT;
    // allocate buffer for a set of interleaved channels
    size_t bufSize = stream->fSize * stream->numChans;
    uint8_t *nbuf = (uint8_t *)alloca(bufSize);
    // assemble interleaved buffer for each element and write to network via stdio buffering/TCP back pressure
    size_t elems=0;
    int noff=0;
    int boff=0;
    while (elems<numElems) {
        for (int c=0; c<stream->numChans; ++c) {
            uint8_t *buf = (uint8_t *)buffs[c];
            memcpy(nbuf+noff, buf+boff, stream->fSize);
            noff += stream->fSize;
        }
        boff += stream->fSize;
        if (fwrite(nbuf, bufSize, 1, stream->dataFp)!=1) {
            SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::writeStream, error writing data: %s", strerror(errno));
            break;
        }
        ++elems;
    }
    return (int)elems;
}

int SoapyTCPRemote::readStreamStatus(
                    SoapySDR::Stream *stream,
                    size_t &chanMask,
                    int &flags,
                    long long &timeNs,
                    const long timeoutUs)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::readStreamStatus()");
    // TODO: report something?
    return SOAPY_SDR_NOT_SUPPORTED;
}

std::vector<std::string> SoapyTCPRemote::listGains(const int direction, const size_t channel) const
{
    //list available gain element names,
    //the functions below have a "name" parameter
    int status = rpc->writeInteger(TCPREMOTE_LIST_GAINS);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
     return rpc->readStrVector();
}

bool SoapyTCPRemote::hasGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::hasGainMode()");
    int status = rpc->writeInteger(TCPREMOTE_HAS_GAIN_MODE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->readInteger();
    return status!=0;
}

void SoapyTCPRemote::setGainMode(const int direction, const size_t channel, const bool automatic)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::setGainMode()");
    int status = rpc->writeInteger(TCPREMOTE_SET_GAIN_MODE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeInteger(automatic?1:0);
}

bool SoapyTCPRemote::getGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGainMode()");
    int status = rpc->writeInteger(TCPREMOTE_GET_GAIN_MODE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return rpc->readInteger()>0;
}

void SoapyTCPRemote::setGain(const int direction, const size_t channel, const double value)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::setGain()");
    int status = rpc->writeInteger(TCPREMOTE_SET_GAIN);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeInteger((int)value);
}

void SoapyTCPRemote::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::setGain(named)");
    int status = rpc->writeInteger(TCPREMOTE_SET_GAIN_NAMED);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeString(name);
    if (status>0)
        status = rpc->writeInteger((int)value);
}

double SoapyTCPRemote::getGain(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGain()");
    int status = rpc->writeInteger(TCPREMOTE_GET_GAIN);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return (double)rpc->readInteger();
}

double SoapyTCPRemote::getGain(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGain(named)");
    int status = rpc->writeInteger(TCPREMOTE_GET_GAIN_NAMED);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeString(name);
    return (double)rpc->readInteger();
}

SoapySDR::Range SoapyTCPRemote::getGainRange(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGainRange()");
    int status = rpc->writeInteger(TCPREMOTE_GET_GAIN_RANGE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return SoapySDR::Range(
        (double)rpc->readInteger(),
        (double)rpc->readInteger(),
        (double)rpc->readInteger()
    );
}

SoapySDR::Range SoapyTCPRemote::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGainRange(named)");
    int status = rpc->writeInteger(TCPREMOTE_GET_GAIN_RANGE_NAMED);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeString(name);
    return SoapySDR::Range(
        (double)rpc->readInteger(),
        (double)rpc->readInteger(),
        (double)rpc->readInteger()
    );
}

// Frequency
void SoapyTCPRemote::setFrequency(const int direction,
                              const size_t channel,
                              const double frequency,
                              const SoapySDR::Kwargs &args)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::setFrequency(%f)", frequency);
    int status = rpc->writeInteger(TCPREMOTE_SET_FREQUENCY);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeInteger((int)frequency);
    if (status>0)
        status = rpc->writeKwargs(args);
}

void SoapyTCPRemote::setFrequency(const int direction,
                              const size_t channel,
                              const std::string &name,
                              const double frequency,
                              const SoapySDR::Kwargs &args)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::setFrequency(%s,%f)", name.c_str(), frequency);
    int status = rpc->writeInteger(TCPREMOTE_SET_FREQUENCY_NAMED);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeString(name);
    if (status>0)
        status = rpc->writeInteger((int)frequency);
    if (status>0)
        status = rpc->writeKwargs(args);
}

double SoapyTCPRemote::getFrequency(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequency()");
    int status = rpc->writeInteger(TCPREMOTE_GET_FREQUENCY);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return (double)rpc->readInteger();
}

double SoapyTCPRemote::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequency(%s)", name.c_str());
    int status = rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_NAMED);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeString(name);
    return (double)rpc->readInteger();
}

std::vector<std::string> SoapyTCPRemote::listFrequencies(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::listFrequencies");
    int status = rpc->writeInteger(TCPREMOTE_LIST_FREQUENCIES);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return rpc->readStrVector();
}
SoapySDR::RangeList SoapyTCPRemote::getFrequencyRange(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequencyRange()");
    int status = rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_RANGE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    // read triplets of beg/end/step until step<=0
    SoapySDR::RangeList list;
    while (true) {
        int beg = rpc->readInteger();
        int end = rpc->readInteger();
        int step = rpc->readInteger();
        if (step<=0)
            break;
        list.push_back(SoapySDR::Range((double)beg,(double)end,(double)step));
    }
    return list;
}


SoapySDR::RangeList SoapyTCPRemote::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequencyRange(%s)", name.c_str());
    int status = rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_RANGE_NAMED);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeString(name);
    // read triplets of beg/end/step until step<=0
    SoapySDR::RangeList list;
    while (true) {
        int beg = rpc->readInteger();
        int end = rpc->readInteger();
        int step = rpc->readInteger();
        if (step<=0)
            break;
       list.push_back(SoapySDR::Range((double)beg,(double)end,(double)step));
    }
    return list;
}

SoapySDR::ArgInfoList SoapyTCPRemote::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequencyRange()");
    int status = rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_ARGS_INFO);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    // TODO: parse complex structure!
    SoapySDR::ArgInfoList list;
    while (status>0) {
        // fugly complex structure to serialise... maybe later!
        std::string s = rpc->readString();
        if (s.length()==0)
            break;
    }
    SoapySDR_log(SOAPY_SDR_ERROR, "Unimplemented: getFrequencyArgsInfo");
    return list;
}

void SoapyTCPRemote::setSampleRate(const int direction, const size_t channel, const double rate)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::setSampleRate(%f)", rate);
    int status = rpc->writeInteger(TCPREMOTE_SET_SAMPLE_RATE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    if (status>0)
        status = rpc->writeInteger((int)rate);
}

double SoapyTCPRemote::getSampleRate(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getSampleRate()");
    int status = rpc->writeInteger(TCPREMOTE_GET_SAMPLE_RATE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    return (double)rpc->readInteger();
}

std::vector<double> SoapyTCPRemote::listSampleRates(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::listSampleRates()");
    // emulate through current API
    SoapySDR::RangeList list = getSampleRateRange(direction, channel);
    std::vector<double> rates;
    for (auto r: list) {
        for (double f=r.minimum(); f<=r.maximum(); f+=r.step())
            rates.push_back(f);
    }
    return rates;
}

SoapySDR::RangeList SoapyTCPRemote::getSampleRateRange(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getSampleRateRange()");
    int status = rpc->writeInteger(TCPREMOTE_GET_SAMPLE_RATE_RANGE);
    if (status>0)
        status = rpc->writeInteger(direction);
    if (status>0)
        status = rpc->writeInteger(channel);
    SoapySDR::RangeList list;
    // read triplets of beg/end/step until step<=0
    while (true) {
        int beg = rpc->readInteger();
        int end = rpc->readInteger();
        int step = rpc->readInteger();
        if (step<=0)
        break;
        list.push_back(SoapySDR::Range((double)beg,(double)end,(double)step));
    }
    return list;
}

SoapySDR::KwargsList findTCPRemote(const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "findTCPRemote");
    
    SoapySDR::KwargsList results;
    
    // We MUST have:
    // tcpremote:address - server IP:Port
    // tcpremote:driver  - driver to load at the remote end
    // We MAY have:
    // tcpremote:args    - driver arguments when creating
    // We ensure mandatory args are present, and parse them
    if (args.find("tcpremote:address")==args.end()) {
        SoapySDR_log(SOAPY_SDR_ERROR, "Missing tcpremote:address");
        return results;
    }
    if (args.find("tcpremote:driver")==args.end()) {
        SoapySDR_log(SOAPY_SDR_ERROR, "Missing tcpremote:driver");
        return results;
    }
    std::string address = args.at("tcpremote:address");
    size_t colon = address.find(':',0);
    // no port, default to 0x50AF (20655)
    int port = 0x50AF;
    if (colon != (size_t)-1) {
        port = atoi(address.substr(colon+1).c_str());
        address = address.substr(0, colon);
    }
    SoapySDR_logf(SOAPY_SDR_TRACE, "findTCPRemote parsed: address=%s port=%u", address.c_str(), port);
    SoapySDR::Kwargs soapyInfo;
    // This is the name that shows up.
    soapyInfo["device"] = "TCP remote: "+address;
    soapyInfo["address"] = address;
    soapyInfo["port"] = std::to_string(port);
    soapyInfo["tcpremote:driver"] = args.at("tcpremote:driver");
    if (args.find("tcpremote:args")!=args.end())
        soapyInfo["tcpremote:args"] = args.at("tcpremote:args");
    else
        soapyInfo["tcpremote:args"] = "";
    results.push_back(soapyInfo);
    return results;
}

SoapySDR::Device *makeTCPRemote(const SoapySDR::Kwargs &args)
{
    SoapySDR_log(SOAPY_SDR_INFO, "makeTCPRemote");
    
    std::string address = args.at("address");
    std::string port = args.at("port");
    std::string remdriver = args.at("tcpremote:driver");
    std::string remargs = args.at("tcpremote:args");
    return (SoapySDR::Device*) new SoapyTCPRemote(address, port, remdriver, remargs);
}

/* Register this driver */
static SoapySDR::Registry registerTCPRemote("tcpremote", &findTCPRemote, &makeTCPRemote, SOAPY_SDR_ABI_VERSION);
