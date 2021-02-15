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
    int netSock;
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
        rpc->writeInteger(TCPREMOTE_DROP_RPC);
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
    rpc->writeInteger(TCPREMOTE_RPC_LOAD);
    rpc->writeString(remoteDriver);
    rpc->writeString(remoteArgs);
    return rpc->readInteger();
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
void SoapyTCPRemote::setFrontendMapping(const int direction, const std::string &mapping)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::setFrontendMapping()");
    rpc->writeInteger(TCPREMOTE_SET_FRONTEND_MAPPING);
    rpc->writeInteger(direction);
    rpc->writeString(mapping);
    rpc->readInteger(); // wait for completion!
}

std::string SoapyTCPRemote::getFrontendMapping(const int direction) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrontendMapping()");
    rpc->writeInteger(TCPREMOTE_GET_FRONTEND_MAPPING);
    rpc->writeInteger(direction);
    return rpc->readString();
}

size_t SoapyTCPRemote::getNumChannels(const int dir) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getNumChannels()");
    rpc->writeInteger(TCPREMOTE_GET_NUM_CHANNELS);
    rpc->writeInteger(dir);
    return rpc->readInteger();
}

SoapySDR::Kwargs SoapyTCPRemote::getChannelInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getChannelInfo()");
    rpc->writeInteger(TCPREMOTE_GET_CHANNEL_INFO);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readKwargs();
}

bool SoapyTCPRemote::getFullDuplex(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFullDuplex()");
    rpc->writeInteger(TCPREMOTE_GET_FULL_DUPLEX);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readInteger()>0;
}

std::vector<std::string> SoapyTCPRemote::getStreamFormats(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getStreamFormats()");
    rpc->writeInteger(TCPREMOTE_GET_STREAM_FORMATS);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readStrVector();
}

std::string SoapyTCPRemote::getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getNativeStreamFormat()");
    rpc->writeInteger(TCPREMOTE_GET_STREAM_NATIVE_FORMAT);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    std::string fmt = rpc->readString();
    fullScale = rpc->readDouble();
   return fmt;
}

SoapySDR::ArgInfoList SoapyTCPRemote::getStreamArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getStreamArgsInfo()");
    rpc->writeInteger(TCPREMOTE_GET_STREAM_ARGS_INFO);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    SoapySDR::ArgInfoList info;
    while (true) {
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
    // grab the native format
    double fs;
    std::string fmtnat = getNativeStreamFormat(direction, channels[0], fs);
    // first check we have a frame size for both formats
    if (g_frameSizes.find(format)==g_frameSizes.end()) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, unknown requested format (%s)", format.c_str());
        return nullptr;
    }
    if (g_frameSizes.find(fmtnat)==g_frameSizes.end()) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, unknown native format (%s)", fmtnat.c_str());
        return nullptr;
    }
    // choose smallest wire format..
    if (g_frameSizes.at(fmtnat)<g_frameSizes.at(format)) {
        fmtwire = fmtnat;
        fmtout = format;
    } else {
        fmtwire = format;
        fmtout = format;
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
    rv->netSock = data;
    rv->fSize = g_frameSizes.at(fmtwire);
    rv->numChans = channels.size();
    rv->running = false;
    /* ensure large receive buffer to avoid blocking transmitter..
    size_t sockBuf = rv->fSize*rv->numChans*(int)this->rate*2;
    if (setsockopt(data, SOL_SOCKET, SO_RCVBUF, &sockBuf, sizeof(sockBuf))) {
        SoapySDR_log(SOAPY_SDR_WARNING, "SoapyTCPRemote::setupStream, unable to set receive buffer size, may impact performance");
    }*/
    // make the RPC call with the remoteId
    rpc->writeInteger(TCPREMOTE_SETUP_STREAM);
    rpc->writeInteger(rv->remoteId);
    rpc->writeInteger(direction);
    rpc->writeString(fmtwire);
    // channel list, sent as a space separated list of numbers on one line
    std::string chans;
    for (auto it=channels.begin(); it!=channels.end(); ++it) {
        if (chans.length()>0)
            chans += " ";
        chans += std::to_string(*it);
    }
    rpc->writeString(chans);
    rpc->writeKwargs(args);
    int status = rpc->readInteger();
    if (status>=0) {
        SoapySDR_logf(SOAPY_SDR_TRACE,"SoapyTCPRemote::setupStream, data stream remoteId: %d", rv->remoteId);
    } else {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::setupStream, error: %d", status);
        if (rv)
            closeStream(rv);
        rv = nullptr;
    }
    return rv;
}

void SoapyTCPRemote::closeStream(SoapySDR::Stream *stream)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::closeStream()");
    if (stream->running)
        deactivateStream(stream);
    rpc->writeInteger(TCPREMOTE_CLOSE_STREAM);
    rpc->writeInteger(stream->remoteId);
    rpc->readInteger(); // ignore return value, but wait!
    close(stream->netSock);
    delete stream;
}

size_t SoapyTCPRemote::getStreamMTU(SoapySDR::Stream *stream) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getStreamMTU()");
    rpc->writeInteger(TCPREMOTE_GET_STREAM_MTU);
    rpc->writeInteger(stream->remoteId);
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
    rpc->writeInteger(TCPREMOTE_ACTIVATE_STREAM);
    rpc->writeInteger(stream->remoteId);
    int status = rpc->readInteger();
    if (status==0)
        stream->running = true;
    return status;
}

int SoapyTCPRemote::deactivateStream(SoapySDR::Stream *stream, const int flags, const long long timeNs)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::deactivateStream()");
    if (!stream->running)
        return 0;
    rpc->writeInteger(TCPREMOTE_DEACTIVATE_STREAM);
    rpc->writeInteger(stream->remoteId);
    int status = rpc->readInteger();
    if (status==0)
        stream->running = false;
    return status;
}

static void *convertCS16toCF32(void *dst, const void *src, size_t cnt /* ignored */)
{
    int16_t *cs16 = (int16_t *)src;
    float *cf32   = (float *)dst;
    // convert complex sample in range -INT16_MAX>INT16_MAX to -1.0>1.0
    int16_t si = cs16[0];
    int16_t sq = cs16[1];
    cf32[0]   = (float)si/(float)INT16_MAX;
    cf32[1] = (float)sq/(float)INT16_MAX;
    return dst;
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
    // We read by making one syscall for the maximum amount, then de-interleaving and possibly
    // converting formats into buffs.
    size_t blkSize = stream->fSize * stream->numChans;
    uint8_t *swamp = (uint8_t *)alloca(blkSize * numElems);
    int status = read(stream->netSock, swamp, blkSize*numElems);
    if (status<=0) {
        SoapySDR_logf(SOAPY_SDR_ERROR, "SoapyTCPRemote::readStream, error reading data: %s", strerror(errno));
        return SOAPY_SDR_STREAM_ERROR;
    }
    status /= stream->fSize;
    int elems=0;
    int soff=0;
    int boff=0;
    int bSize=stream->fSize;
    void *(*cnv)(void *, const void *, size_t) = memcpy;
    if (fmtout!=fmtwire) {
        bSize = g_frameSizes.at(fmtout);
        // TODO: actually check these formats =)
        cnv = convertCS16toCF32;
    }
    while (elems<status) {
        for (int c=0; c<stream->numChans; ++c) {
            uint8_t *buf = (uint8_t *)(buffs[c]);
            cnv(buf+boff, swamp+soff, stream->fSize);
            soff += stream->fSize;
        }
        boff += bSize;
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
        if (write(stream->netSock, nbuf, bufSize)!=(int)bufSize) {
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
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::listGains()");
    rpc->writeInteger(TCPREMOTE_LIST_GAINS);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
     return rpc->readStrVector();
}

bool SoapyTCPRemote::hasGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::hasGainMode()");
    rpc->writeInteger(TCPREMOTE_HAS_GAIN_MODE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readInteger()>0;
}

void SoapyTCPRemote::setGainMode(const int direction, const size_t channel, const bool automatic)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::setGainMode()");
    rpc->writeInteger(TCPREMOTE_SET_GAIN_MODE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeInteger(automatic?1:0);
    rpc->readInteger(); // wait for completion!
}

bool SoapyTCPRemote::getGainMode(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGainMode()");
    rpc->writeInteger(TCPREMOTE_GET_GAIN_MODE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readInteger()>0;
}

void SoapyTCPRemote::setGain(const int direction, const size_t channel, const double value)
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::setGain()");
    rpc->writeInteger(TCPREMOTE_SET_GAIN);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeDouble(value);
    rpc->readInteger(); // wait for completion!
}

void SoapyTCPRemote::setGain(const int direction, const size_t channel, const std::string &name, const double value)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::setGain(%s)", name.c_str());
    rpc->writeInteger(TCPREMOTE_SET_GAIN_NAMED);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeString(name);
    rpc->writeDouble(value);
    rpc->readInteger(); // wait for completion!
}

double SoapyTCPRemote::getGain(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGain()");
    rpc->writeInteger(TCPREMOTE_GET_GAIN);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readDouble();
}

double SoapyTCPRemote::getGain(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGain(%s)", name.c_str());
    rpc->writeInteger(TCPREMOTE_GET_GAIN_NAMED);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeString(name);
    return rpc->readDouble();
}

SoapySDR::Range SoapyTCPRemote::getGainRange(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGainRange()");
    rpc->writeInteger(TCPREMOTE_GET_GAIN_RANGE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return SoapySDR::Range(
        rpc->readDouble(),
        rpc->readDouble(),
        rpc->readDouble()
    );
}

SoapySDR::Range SoapyTCPRemote::getGainRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::getGainRange(%s)", name.c_str());
    rpc->writeInteger(TCPREMOTE_GET_GAIN_RANGE_NAMED);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeString(name);
    return SoapySDR::Range(
        rpc->readDouble(),
        rpc->readDouble(),
        rpc->readDouble()
    );
}

// Frequency
void SoapyTCPRemote::setFrequency(const int direction,
                              const size_t channel,
                              const double frequency,
                              const SoapySDR::Kwargs &args)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::setFrequency(%f)", frequency);
    rpc->writeInteger(TCPREMOTE_SET_FREQUENCY);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeDouble(frequency);
    rpc->writeKwargs(args);
    rpc->readInteger(); // wait for completion!
}

void SoapyTCPRemote::setFrequency(const int direction,
                              const size_t channel,
                              const std::string &name,
                              const double frequency,
                              const SoapySDR::Kwargs &args)
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::setFrequency(%s,%f)", name.c_str(), frequency);
    rpc->writeInteger(TCPREMOTE_SET_FREQUENCY_NAMED);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeString(name);
    rpc->writeDouble(frequency);
    rpc->writeKwargs(args);
    rpc->readInteger(); // wait for completion!
}

double SoapyTCPRemote::getFrequency(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequency()");
    rpc->writeInteger(TCPREMOTE_GET_FREQUENCY);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readDouble();
}

double SoapyTCPRemote::getFrequency(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequency(%s)", name.c_str());
    rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_NAMED);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeString(name);
    return rpc->readDouble();
}

std::vector<std::string> SoapyTCPRemote::listFrequencies(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::listFrequencies()");
    rpc->writeInteger(TCPREMOTE_LIST_FREQUENCIES);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readStrVector();
}
SoapySDR::RangeList SoapyTCPRemote::getFrequencyRange(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequencyRange()");
    rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_RANGE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    // read triplets of beg/end/step until step<0
    SoapySDR::RangeList list;
    while (true) {
        double beg = rpc->readDouble();
        double end = rpc->readDouble();
        double step = rpc->readDouble();
        if (step<0)
            break;
        list.push_back(SoapySDR::Range(beg,end,step));
    }
    return list;
}


SoapySDR::RangeList SoapyTCPRemote::getFrequencyRange(const int direction, const size_t channel, const std::string &name) const
{
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequencyRange(%s)", name.c_str());
    rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_RANGE_NAMED);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeString(name);
    // read triplets of beg/end/step until step<0
    SoapySDR::RangeList list;
    while (true) {
        double beg = rpc->readDouble();
        double end = rpc->readDouble();
        double step = rpc->readDouble();
        if (step<0)
            break;
        list.push_back(SoapySDR::Range(beg,end,step));
    }
    return list;
}

SoapySDR::ArgInfoList SoapyTCPRemote::getFrequencyArgsInfo(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getFrequencyArgsInfo()");
    rpc->writeInteger(TCPREMOTE_GET_FREQUENCY_ARGS_INFO);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    // TODO: parse complex structure!
    SoapySDR::ArgInfoList list;
    while (true) {
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
    rpc->writeInteger(TCPREMOTE_SET_SAMPLE_RATE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    rpc->writeDouble(rate);
    rpc->readInteger(); // wait for completion!
}

double SoapyTCPRemote::getSampleRate(const int direction, const size_t channel) const
{
    SoapySDR_log(SOAPY_SDR_TRACE, "SoapyTCPRemote::getSampleRate()");
    rpc->writeInteger(TCPREMOTE_GET_SAMPLE_RATE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    return rpc->readDouble();
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
    rpc->writeInteger(TCPREMOTE_GET_SAMPLE_RATE_RANGE);
    rpc->writeInteger(direction);
    rpc->writeInteger(channel);
    SoapySDR::RangeList list;
    // read triplets of beg/end/step until step<0
    while (true) {
        double beg = rpc->readDouble();
        double end = rpc->readDouble();
        double step = rpc->readDouble();
        if (step<0)
            break;
        list.push_back(SoapySDR::Range(beg,end,step));
    }
    return list;
}

std::string getConfFile() {
    // We support a configuration file in one of:
    // [$XDG_CONFIG_DIRS]/SoapyTCPRemote.conf, $HOME/.config/SoapyTCPRemote.conf
    SoapySDR_log(SOAPY_SDR_TRACE, "getConfFile");
    std::string tst="";
    if (getenv("XDG_CONFIG_DIRS")!=nullptr) {
        std::string env = getenv("XDG_CONFIG_DIRS");
        size_t pos, nxt=-1;
        do {
            pos = nxt+1;
            size_t nxt = env.find(':', pos);
            tst = env.substr(pos,nxt-pos);
            tst += "/SoapyTCPRemote.conf";
            if (access(tst.c_str(), R_OK)==0)
                goto done;
        } while (nxt!=std::string::npos);
    }
    tst = "/etc/xdg/SoapyTCPRemote.conf";
    if (access(tst.c_str(), R_OK)==0)
        goto done;
    if (getenv("HOME")!=nullptr) {
        tst = getenv("HOME");
        tst += "/.config/SoapyTCPRemote.conf";
        if (access(tst.c_str(), R_OK)==0)
            goto done;
    }
    tst = "";
done:
    SoapySDR_logf(SOAPY_SDR_TRACE, "SoapySDRRemote::getConfFile()=%s", tst.c_str());
    return tst;
}

std::string getConfValue(const std::string &key) {
    SoapySDR_logf(SOAPY_SDR_TRACE, "getConfValue(%s)", key.c_str());
    std::string rv = "";
    std::string cf = getConfFile();
    FILE *fp = nullptr;
    if (cf.length()>0 && (fp=fopen(cf.c_str(), "r"))!=nullptr) {
        char line[80];
        while (fgets(line, sizeof(line), fp)) {
            std::string arg = line;
            if (arg[0]=='#')
                continue;
            arg = arg.substr(0, arg.length()-1);
            size_t off = arg.find(key, 0);
            if (std::string::npos==off)
                continue;
            off = arg.find('=', off);
            if (std::string::npos==off)
                continue;
            rv = arg.substr(off+1);
            break;
        }
        fclose(fp);
    }
    return rv;
}

// NB: this is called with random (can be NULL) parameters when enumeration is done by an app,
// and with *merged* parameters (app & enumeration response) when a device is created by Device::make().
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
    std::string address = getConfValue("address");
    if (args.find("tcpremote:address")==args.end()) {
        SoapySDR_logf(SOAPY_SDR_DEBUG, "Missing tcpremote:address, using: %s", address.c_str());
    } else {
         address = args.at("tcpremote:address");
    }
    std::string driver = getConfValue("driver");
    if (args.find("tcpremote:driver")==args.end()) {
        SoapySDR_logf(SOAPY_SDR_DEBUG, "Missing tcpremote:driver, using: %s", driver.c_str());
    } else {
        driver = args.at("tcpremote:driver");
    }
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
    soapyInfo["tcpremote:driver"] = driver;
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
