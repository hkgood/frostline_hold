/* The Raylib types the PC build compiles against.
 *
 * The Engine's Host header carries only the drawing types; the device takes
 * the audio types from the full upstream Raylib. The PC build plays sound, so
 * it adds those here. CMake points FROSTLINE_HOST_RAYLIB_H at the Host header
 * and puts this directory first, so every "raylib.h" lands on this file.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#pragma once

#include FROSTLINE_HOST_RAYLIB_H

typedef struct rAudioBuffer rAudioBuffer;
typedef struct rAudioProcessor rAudioProcessor;

/* Field for field the upstream layout, so the game reads them the same way
 * on every target. */
typedef struct AudioStream {
    rAudioBuffer *buffer;
    rAudioProcessor *processor;
    unsigned int sampleRate;
    unsigned int sampleSize;
    unsigned int channels;
} AudioStream;

typedef struct Sound {
    AudioStream stream;
    unsigned int frameCount;
} Sound;

typedef struct Music {
    AudioStream stream;
    unsigned int frameCount;
    bool looping;
    int ctxType;
    void *ctxData;
} Music;
