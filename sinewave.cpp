/*
 *  sinewave.cpp  -  Don Cross <cosinekitty@gmail.com>
 *
 *  Test program to reproduce a glitching problem I
 *  encountered originally in VCV Rack 2.1.0+ using rtaudio.
 */

#include <cstdio>
#include <vector>
#include "RtAudio.h"

void errorCallback(RtAudioErrorType type, const std::string &errorText)
{
    printf("errorCallback(%d): %s\n", type, errorText.c_str());
}

int main(int argc, const char *argv[])
{
    RtAudio dac(RtAudio::LINUX_PULSE, &errorCallback);
    if (dac.getCurrentApi() != RtAudio::LINUX_PULSE)
    {
        printf("Could not initialize PulseAudio.\n");
        return 1;
    }

    std::vector<unsigned> deviceIdList = dac.getDeviceIds();
    for (unsigned id : deviceIdList)
    {
        RtAudio::DeviceInfo info = dac.getDeviceInfo(id);
        printf("    %u = [%s]\n", id, info.name.c_str());
    }

    return 0;
}
