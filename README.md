# Glitch in rtaudio + PulseAudio

## Introduction

This project reproduces an audio glitch that occurs
when using rtaudio to generate audio via PulseAudio on Linux.

I discovered this using VCV Rack 2.1.0+ and [reported it on the VCV Rack forum](https://community.vcvrack.com/t/clicks-pops-using-vcv-rack-2-1-0-edit-was-2-2-0/18796).
I subsequently determined the root cause to be a change in how rtaudio initializes PulseAudio,
and [submitted this fix](https://github.com/cosinekitty/rtaudio/commit/effb23d8c64efe00c741ed746ca0e0f4e983f17a) to the VCV Rack fork of rtaudio.

Andrew Belt suggested I report the issue and the proposed fix to Gary Scavone at
his original [rtaudio repo on GitHub](https://github.com/thestk/rtaudio).

This is my first step: a program that demonstrates there really is an issue in PulseAudio
without my fix, and that the problem goes away when my fix is applied.

## Reproducing the glitch

1. Clone this repository on a Linux system that has PulseAudio installed.
2. `cd pulseaudio_glitch_demo`
3. `./run`

You will hear an 80 Hz stereo sine wave. To reproduce the glitch, allow the program
to continue running while you switch to other windows, run other things, etc.
You will hear clicks and pops every now and then.
On my system, the glitches happen even if I do nothing, but they happen a lot
more if I open Google Chrome and start switching tabs around.
But pretty much any normal user activity will cause the glitches sooner or later.

## Validating the fix
Apply my patch to rtaudio, then repeat the steps above.
The glitch will disappear.
