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

    unsigned outputDeviceId = 0;

    if (argc > 1)
    {
        if (1 != sscanf(argv[1], "%u", &outputDeviceId))
        {
            printf("ERROR: Invalid device ID '%s' on command line.\n", argv[1]);
            return 1;
        }
        RtAudio::DeviceInfo info = dac.getDeviceInfo(outputDeviceId);
        if (info.name.empty())
        {
            printf("ERROR: Unknown device ID %u\n", outputDeviceId);
            return 1;
        }
        printf("Device %u = [%s]\n", outputDeviceId, info.name.c_str());
    }
    else
    {
        bool found = false;
        std::vector<unsigned> deviceIdList = dac.getDeviceIds();
        for (unsigned id : deviceIdList)
        {
            RtAudio::DeviceInfo info = dac.getDeviceInfo(id);
            printf("    %u = [%s]\n", id, info.name.c_str());
            if (info.isDefaultOutput)
            {
                outputDeviceId = id;
                found = true;
            }
        }

        if (!found)
        {
            printf("Could not find default output device.\n");
            return 1;
        }

        printf("Found default output device ID = %d\n", outputDeviceId);
    }

    return 0;
}
