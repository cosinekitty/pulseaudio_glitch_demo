/*
 *  sinewave.cpp  -  Don Cross <cosinekitty@gmail.com>
 *
 *  Test program to reproduce a glitching problem I
 *  encountered originally in VCV Rack 2.1.0+ using rtaudio.
 */

#include <cstdio>
#include <vector>
#include "RtAudio.h"

int main(int argc, const char *argv[])
{
    std::vector<RtAudio::Api> apis;
    RtAudio::getCompiledApi(apis);
    for (RtAudio::Api api : apis)
        printf("Found API: %d\n", api);
    return 0;
}
