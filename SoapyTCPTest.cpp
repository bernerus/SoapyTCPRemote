//  SoapyTCPTest.cpp
//  Copyright (c) 2021 Phil Ashby
//  SPDX-License-Identifier: BSL-1.0

#include <stdio.h>
#include <SoapySDR/Device.hpp>

int main(int argc, char **argv) {
    puts("Aloha!");
    // enumerate everything
    SoapySDR::KwargsList devs = SoapySDR::Device::enumerate("");
    // find tcpremote and print args
    for (auto dev: devs) {
        for (auto kv=dev.begin(); kv!=dev.end(); ++kv) {
            printf("%s:\t%s\n", (*kv).first.c_str(), (*kv).second.c_str());
        }
        puts("");
    }
    return 0;
}
