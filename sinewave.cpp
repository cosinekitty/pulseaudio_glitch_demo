/*
 *  sinewave.cpp  -  Don Cross <cosinekitty@gmail.com>
 *
 *  Test program to reproduce a glitching problem I
 *  encountered originally in VCV Rack 2.1.0+ using rtaudio.
 */

#include <cstdio>
#include <cmath>
#include <vector>
#include "RtAudio.h"

void errorCallback(RtAudioErrorType type, const std::string &errorText)
{
    printf("errorCallback(%d): %s\n", type, errorText.c_str());
}

class SinewaveGenerator
{
private:
    const unsigned sampleRate;
    float a;    // real component of phasor
    float b;    // imaginary component of phasor
    float c;    // cosine of angle increment
    float s;    // sine of angle increment

    void generate(float *buffer, int nFrames)
    {
        for (int i = 0; i < nFrames; ++i)
        {
            buffer[2*i] = a;
            buffer[2*i + 1] = b;
            float t = a*c - b*s;
            b = a*s + b*c;
            a = t;
        }
    }

public:
    SinewaveGenerator(unsigned _sampleRate, float _frequencyHz)
        : sampleRate(_sampleRate)
        , a(1.0f)
        , b(0.0f)
    {
        float radians = (_frequencyHz * M_PI * 2.0) / _sampleRate;
        c = std::cos(radians);
        s = std::sin(radians);
    }

    static int callback(
        void *outputBuffer,
        void *inputBuffer,
        unsigned int nFrames,
        double streamTime,
        RtAudioStreamStatus status,
        void *userData)
    {
        static_cast<SinewaveGenerator*>(userData)->generate(
            static_cast<float *>(outputBuffer),
            nFrames
        );
        return 0;
    }
};

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
        // Use the device ID specified on the command line.
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
        // Find the default output device.
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

    // Render audio to the selected output device.
    RtAudio::StreamParameters outputParameters;
    outputParameters.deviceId = outputDeviceId;
    outputParameters.firstChannel = 0;
    outputParameters.nChannels = 2;

    const unsigned sampleRate = 44100;
    const float frequencyHz = 80.0f;
    unsigned bufferFrames = 256;

    SinewaveGenerator generator {sampleRate, frequencyHz};

    RtAudio::StreamOptions options;
    options.flags |= RTAUDIO_SCHEDULE_REALTIME;
    options.numberOfBuffers = 2;
    options.streamName = "Sinewave Generator";

    RtAudioErrorType error = dac.openStream(
        &outputParameters,
        nullptr,
        RTAUDIO_FLOAT32,
        sampleRate,
        &bufferFrames,
        SinewaveGenerator::callback,
        &generator,
        &options
    );

    if (error != RTAUDIO_NO_ERROR)
    {
        printf("openStream returned error %d\n", error);
        return 1;
    }

    error = dac.startStream();
    if (error != RTAUDIO_NO_ERROR)
    {
        printf("startStream returned error %d\n", error);
        return 1;
    }

    printf("Generating audio. Press ENTER to quit.\n");
    char line[80];
    fgets(line, sizeof(line), stdin);

    dac.closeStream();

    return 0;
}
