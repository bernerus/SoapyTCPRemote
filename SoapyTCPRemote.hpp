//  SoapyTCPRemote.hpp
//  Copyright (c) 2021 Phil Ashby
//  SPDX-License-Identifier: BSL-1.0

#ifndef SoapyTCPRemote_hpp
#define SoapyTCPRemote_hpp

#include <SoapySDR/Device.hpp>
#include <SoapySDR/Registry.hpp>
#include <SoapySDR/Version.hpp>

#include "SoapyRPC.hpp"

class SoapyTCPRemote : public SoapySDR::Device
{

private:
    // connection details & remote driver + any settings
    const std::string remoteAddress;
    const std::string remotePort;
    const std::string remoteDriver;
    const std::string remoteArgs;
    // network connect
    int connect() const;
    // RPC handler
    SoapyRPC *rpc;
    // helpers
    int loadRemoteDriver() const;
public:
    SoapyTCPRemote(const std::string &address, const std::string &port, const std::string &remdriver, const std::string &remargs);
    ~SoapyTCPRemote();

    // Identification API (driver local, others remote)
    std::string getDriverKey() const { return "tcpRemote"; }
    std::string getHardwareKey() const;
    SoapySDR::Kwargs getHardwareInfo(void) const;

    // Channels API (most remote)
    size_t getNumChannels(const int dir) const;
    SoapySDR::Kwargs getChannelInfo(const int direction, const size_t channel) const;
    // NB: we only support RX, so this always returns false
    bool getFullDuplex(const int direction, const size_t channel) const { return false; }

    // Stream API (all remote)
    std::vector<std::string> getStreamFormats(const int direction, const size_t channel) const;
    std::string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const;
    SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const;
    SoapySDR::Stream* setupStream(const int direction, const std::string &format, const std::vector<size_t> &channels = std::vector<size_t>(), const SoapySDR::Kwargs &args = SoapySDR::Kwargs());

    void closeStream(SoapySDR::Stream *stream);
    size_t getStreamMTU(SoapySDR::Stream *stream) const;
    int activateStream(SoapySDR::Stream *stream,
                       const int flags = 0,
                       const long long timeNs = 0,
                       const size_t numElems = 0);
    int deactivateStream(SoapySDR::Stream *stream, const int flags = 0, const long long timeNs = 0);

    int readStream(SoapySDR::Stream *stream,
                   void * const *buffs,
                   const size_t numElems,
                   int &flags,
                   long long &timeNs,
                   const long timeoutUs = 100000);
    // writeStream - not implemented
    // readStreamStatus - not implemented

    // Direct Buffer Access API (local, and nope!)
    size_t getNumDirectAccessBuffers(SoapySDR::Stream *stream) const { return 0; }

    // Antennas (all remote)
    std::vector<std::string> listAntennas(const int direction, const size_t channel) const;
    void setAntenna(const int direction, const size_t channel, const std::string &name);
    std::string getAntenna(const int direction, const size_t channel) const;

    // DC offset, IQ balance & Frequency correction (local, nope)
    bool hasDCOffsetMode(const int direction, const size_t channel) const { return false; }
    bool hasDCOffset(const int direction, const size_t channel) const { return false; }
    bool hasIQBalanceMode(const int direction, const size_t channel) const { return false; }
    bool hasIQBalance(const int direction, const size_t channel) const { return false; }
    bool hasFrequencyCorrection(const int direction, const size_t channel) const { return false; }

    // Gain (all remote)
    std::vector<std::string> listGains(const int direction, const size_t channel) const;
    bool hasGainMode(const int direction, const size_t channel) const;
    void setGainMode(const int direction, const size_t channel, const bool automatic);
    bool getGainMode(const int direction, const size_t channel) const;
    void setGain(const int direction, const size_t channel, const std::string &name, const double value);
    double getGain(const int direction, const size_t channel, const std::string &name) const;
    SoapySDR::Range getGainRange(const int direction, const size_t channel, const std::string &name) const;

    // Frequency (all remote)
    void setFrequency(const int direction,
                      const size_t channel,
                      const std::string &name,
                      const double frequency,
                      const SoapySDR::Kwargs &args = SoapySDR::Kwargs());
    double getFrequency(const int direction, const size_t channel, const std::string &name) const;
    std::vector<std::string> listFrequencies(const int direction, const size_t channel) const;
    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel, const std::string &name) const;
    // local and nothing to see here
    SoapySDR::ArgInfoList getFrequencyArgsInfo(const int direction, const size_t channel) const { SoapySDR::ArgInfoList l; return l; }

    // Sample Rate API (all remote)
    void setSampleRate(const int direction, const size_t channel, const double rate);
    double getSampleRate(const int direction, const size_t channel) const;
    std::vector<double> listSampleRates(const int direction, const size_t channel) const;

    // Bandwidth, Clocking, Time, Sensor, Register & Setting APIs (not implemented)
};

#endif /* SoapyTCPRemote_hpp */
