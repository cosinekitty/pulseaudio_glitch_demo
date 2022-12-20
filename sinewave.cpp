/*
 *  sinewave.cpp  -  Don Cross <cosinekitty@gmail.com>
 *
 *  Test program to reproduce a glitching problem I
 *  encountered originally in VCV Rack 2.1.0+ using rtaudio.
 */

#include <cstdio>
#include <cmath>
#include <vector>
#include <unistd.h>
#include "RtAudio.h"

void errorCallback(RtAudioErrorType type, const std::string &errorText)
{
    printf("errorCallback(%d): %s\n", type, errorText.c_str());
}


const unsigned FADE_FRAMES = 110;

class SinewaveGenerator
{
private:
    const unsigned sampleRate;
    float a;    // real component of phasor
    float b;    // imaginary component of phasor
    float c;    // cosine of angle increment
    float s;    // sine of angle increment
    unsigned fadeOutCount = 0;
    unsigned fadeInCount = FADE_FRAMES;

    int generate(float *buffer, int nFrames)
    {
        int result = 0;     // continue generator audio

        for (int i = 0; i < nFrames; ++i)
        {
            if (result == 1)
            {
                // We have already faded out completely.
                // Emit silence from this point on.
                buffer[2*i] = buffer[2*i+1] = 0.0f;
            }
            else
            {
                // Use complex number multiplication to generate the next point on the unit circle.
                buffer[2*i] = a;
                buffer[2*i + 1] = b;
                float t = a*c - b*s;
                b = a*s + b*c;
                a = t;

                // Fade in when we start generating audio, to prevent initial click.
                if (fadeInCount > 0)
                {
                    float scale = static_cast<float>(FADE_FRAMES - fadeInCount) / FADE_FRAMES;
                    buffer[2*i] *= scale;
                    buffer[2*i + 1] *= scale;
                    --fadeInCount;
                }

                // Fade out when we stop generating audio, to prevent final click.
                if (fadeOutCount > 0)
                {
                    float scale = static_cast<float>(fadeOutCount) / FADE_FRAMES;
                    buffer[2*i] *= scale;
                    buffer[2*i + 1] *= scale;
                    --fadeOutCount;
                    if (fadeOutCount == 0)
                        result = 1;     // stop generating audio after this
                }
            }
        }

        return result;
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
        return static_cast<SinewaveGenerator*>(userData)->generate(
            static_cast<float *>(outputBuffer),
            nFrames
        );
    }

    void startFadeOut()
    {
        fadeOutCount = FADE_FRAMES;
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
    //options.flags |= RTAUDIO_SCHEDULE_REALTIME;
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

    // Ramp down the audio generation.
    // The generator will signal completion of the stream when it fades out.
    generator.startFadeOut();

    // Wait for audio to drain.
    while (dac.isStreamRunning())
        usleep(1000);

    // All done!
    dac.closeStream();
    return 0;
}
