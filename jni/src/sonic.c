/* Sonic library
   Copyright 2010
   Bill Cox
   This file is part of the Sonic Library.

   This file is licensed under the Apache 2.0 license.
*/

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jni.h>
#include "sonic.h"
#include "types.h"

struct sonicStreamStruct {
    short *inputBuffer;
    short *outputBuffer;
    short *downSampleBuffer;
    float speed;
    u8 numChannels;
    u32 inputBufferSize;
    u32 outputBufferSize;
    u32 numInputSamples;

    u32 numOutputSamples;

    int minPeriod;
    int maxPeriod;
    u32 maxRequired;
    u32 remainingInputToCopy;

    u32 sampleRate;
    int prevPeriod;
    int prevMinDiff;
    u32 frame_cnt_to_sample_cnt_shift;
    bool out_buf_malloced;
};

/* Free stream buffers. */
void freeStreamBuffers(
    sonicStream stream)
{
    free(stream->inputBuffer);
    free(stream->downSampleBuffer);
}

/* Destroy the sonic stream. */
void sonicDestroyStream(
    sonicStream stream)
{
    freeStreamBuffers(stream);
    free(stream);
}

u32 get_frame_cnt_to_byte_cnt_shift(sonicStream stream) {
    return stream->frame_cnt_to_sample_cnt_shift + 1;
}

/* Allocate stream buffers. */
void allocateStreamBuffers(
    sonicStream stream,
    u32 sampleRate,
    u8 numChannels)
{
    u32 minPeriod = sampleRate/SONIC_MAX_PITCH;
    u32 maxPeriod = sampleRate/SONIC_MIN_PITCH;
    u32 maxRequired = 2*maxPeriod;

    stream->inputBufferSize = maxRequired;
    stream->inputBuffer = malloc((size_t) (maxRequired * 2 * numChannels));

    stream->downSampleBuffer = malloc((size_t) (maxRequired * 2));

    stream->sampleRate = sampleRate;
    stream->numChannels = numChannels;
    stream->minPeriod = minPeriod;
    stream->maxPeriod = maxPeriod;
    stream->maxRequired = maxRequired;
    stream->prevPeriod = 0;

    stream->frame_cnt_to_sample_cnt_shift = (numChannels == 2)? 1 : 0;
}

/* Create a sonic stream.  Return NULL only if we are out of memory and cannot
   allocate the stream. */
sonicStream sonicCreateStream(
    u32 sampleRate,
    u8 numChannels)
{
    sonicStream stream = calloc(1, sizeof(struct sonicStreamStruct));

    allocateStreamBuffers(stream, sampleRate, numChannels);

    stream->speed = 1.0f;
    return stream;
}

/* Enlarge the output buffer if needed. */
void enlargeOutputBufferIfNeeded(
    sonicStream stream,
    int frame_cnt)
{
    u32 len_frames = stream->numOutputSamples;
    u32 cap_frames = stream->outputBufferSize;

    if ((len_frames + frame_cnt) > cap_frames) {

        u32 new_cap_frames = cap_frames + (cap_frames >> 1) + frame_cnt;

        i32 shift = get_frame_cnt_to_byte_cnt_shift(stream);

        size_t new_cap_bytes = (size_t) (new_cap_frames << shift);

        i16 *buf = stream->outputBuffer;

        i16 *new_buf;

        if (stream->out_buf_malloced) {
            new_buf = realloc(buf, new_cap_bytes);
        }
        else {
            new_buf = malloc(new_cap_bytes);
            size_t len_bytes = (size_t) (len_frames << shift);

            memcpy(new_buf, buf, len_bytes);

            stream->out_buf_malloced = true;
        }

        stream->outputBuffer = new_buf;
        stream->outputBufferSize = new_cap_frames;
    }
}

/* Enlarge the input buffer if needed. */
void enlargeInputBufferIfNeeded(
    sonicStream stream,
    u32 numSamples)
{
    if (stream->numInputSamples + numSamples > stream->inputBufferSize) {

        stream->inputBufferSize += (stream->inputBufferSize >> 1) + numSamples;
        u32 shift = get_frame_cnt_to_byte_cnt_shift(stream);
        stream->inputBuffer = realloc(stream->inputBuffer,
                                      (size_t) (stream->inputBufferSize << shift));
    }
}

/* Add the input samples to the input buffer. */
void addShortSamplesToInputBuffer(
    sonicStream stream,
    short *samples,
    u32 numSamples)
{
    if (numSamples == 0) {
        return;
    }

    enlargeInputBufferIfNeeded(stream, numSamples);

    memcpy(stream->inputBuffer + (stream->numInputSamples << stream->frame_cnt_to_sample_cnt_shift),
           samples, (size_t) (numSamples << get_frame_cnt_to_byte_cnt_shift(stream)));
    stream->numInputSamples += numSamples;
}

/* Remove input samples that we have already processed. */
void removeInputSamples(
    sonicStream stream,
    int position)
{
    u32 remainingSamples = stream->numInputSamples - position;

    u32 shift_s = stream->frame_cnt_to_sample_cnt_shift;
    u32 shift_b = get_frame_cnt_to_byte_cnt_shift(stream);

    if (remainingSamples != 0) {
        memmove(stream->inputBuffer, stream->inputBuffer + (position << shift_s),
                remainingSamples << shift_b);
    }

    stream->numInputSamples = remainingSamples;
}

/* Just copy from the array to the output buffer */
void copyToOutput(
    sonicStream stream,
    short *samples,
    int numSamples)
{
    enlargeOutputBufferIfNeeded(stream, numSamples);

    int shift = get_frame_cnt_to_byte_cnt_shift(stream);

    memcpy(
        stream->outputBuffer + (stream->numOutputSamples << stream->frame_cnt_to_sample_cnt_shift),
        samples, (size_t) (numSamples << shift));

    stream->numOutputSamples += numSamples;
}

/* Just copy from the input buffer to the output buffer.  Return 0 if we fail to
   resize the output buffer.  Otherwise, return numSamples */
int copyInputToOutput(
    sonicStream stream,
    int position)
{
    u32 numSamples = stream->remainingInputToCopy;

    if(numSamples > stream->maxRequired) {
        numSamples = stream->maxRequired;
    }

    copyToOutput(stream, stream->inputBuffer + position*stream->numChannels,
            numSamples);

    stream->remainingInputToCopy -= numSamples;
    return numSamples;
}

/* Force the sonic stream to generate output using whatever data it currently
   has.  No extra delay will be added to the output, but flushing in the middle of
   words could introduce distortion. */
int sonicFlushStream(
    sonicStream stream)
{
    u32 maxRequired = stream->maxRequired;
    u32 remainingSamples = stream->numInputSamples;
    float speed = stream->speed;

    u32 expectedOutputSamples = stream->numOutputSamples +
        (u32) ((remainingSamples / speed) / 1.5f);

    /* Add enough silence to flush both input and pitch buffers. */
    enlargeInputBufferIfNeeded(stream, remainingSamples + 2*maxRequired);

    memset(stream->inputBuffer + remainingSamples*stream->numChannels, 0,
           (2*maxRequired) << get_frame_cnt_to_byte_cnt_shift(stream));
    stream->numInputSamples += 2*maxRequired;

    changeSpeed(stream);

    /* Throw away any extra samples we generated due to the silence we added */
    if(stream->numOutputSamples > expectedOutputSamples) {
        stream->numOutputSamples = expectedOutputSamples;
    }

    /* Empty input and pitch buffers */
    stream->numInputSamples = 0;
    stream->remainingInputToCopy = 0;

    return 1;
}

void sonic_discard_in_buf(sonicStream stream) {

    stream->numInputSamples = 0;
    stream->remainingInputToCopy = 0;
}

/* If skip is greater than one, average skip samples together and write them to
   the down-sample buffer.  If numChannels is greater than one, mix the channels
   together as we down sample. */
void downSampleInput(
    sonicStream stream,
    short *samples,
    int skip)
{
    int numSamples = stream->maxRequired/skip;
    int samplesPerValue = stream->numChannels*skip;

    short *downSamples = stream->downSampleBuffer;

    for (int i = 0; i < numSamples; i++) {

        int value = 0;

        for(int j = 0; j < samplesPerValue; j++) {
            value += *samples++;
        }

        value /= samplesPerValue;
        downSamples[i] = (short) value;
    }
}

/* Find the best frequency match in the range, and given a sample skip multiple.
   For now, just find the pitch of the first channel. */
int findPitchPeriodInRange(
    short *samples,
    int minPeriod,
    int maxPeriod,
    int *retMinDiff,
    int *retMaxDiff)
{
    int period, bestPeriod = 0, worstPeriod = 255;
    short *s, *p, sVal, pVal;
    unsigned long diff, minDiff = 1, maxDiff = 0;
    int i;

    for(period = minPeriod; period <= maxPeriod; period++) {
        diff = 0;
        s = samples;
        p = samples + period;
        for(i = 0; i < period; i++) {
            sVal = *s++;
            pVal = *p++;
            diff += sVal >= pVal? (unsigned short)(sVal - pVal) :
                (unsigned short)(pVal - sVal);
        }
        /* Note that the highest number of samples we add into diff will be less
           than 256, since we skip samples.  Thus, diff is a 24 bit number, and
           we can safely multiply by numSamples without overflow */
        /* if (bestPeriod == 0 || (bestPeriod*3/2 > period && diff*bestPeriod < minDiff*period) ||
                diff*bestPeriod < (minDiff >> 1)*period) {*/
        if (bestPeriod == 0 || diff*bestPeriod < minDiff*period) {
            minDiff = diff;
            bestPeriod = period;
        }
        if(diff*worstPeriod > maxDiff*period) {
            maxDiff = diff;
            worstPeriod = period;
        }
    }
    *retMinDiff = (int) (minDiff/bestPeriod);
    *retMaxDiff = (int) (maxDiff/worstPeriod);
    return bestPeriod;
}

/* At abrupt ends of voiced words, we can have pitch periods that are better
   approximated by the previous pitch period estimate.  Try to detect this case. */
int prevPeriodBetter(
    sonicStream stream,
    int minDiff,
    int maxDiff,
    int preferNewPeriod)
{
    if(minDiff == 0 || stream->prevPeriod == 0) {
        return 0;
    }
    if(preferNewPeriod) {
        if(maxDiff > minDiff*3) {
            /* Got a reasonable match this period */
            return 0;
        }
        if(minDiff*2 <= stream->prevMinDiff*3) {
            /* Mismatch is not that much greater this period */
            return 0;
        }
    } else {
        if(minDiff <= stream->prevMinDiff) {
            return 0;
        }
    }
    return 1;
}

/* Find the pitch period.  This is a critical step, and we may have to try
   multiple ways to get a good answer.  This version uses Average Magnitude
   Difference Function (AMDF).  To improve speed, we down sample by an integer
   factor get in the 11KHz range, and then do it again with a narrower
   frequency range without down sampling */
int findPitchPeriod(
    sonicStream stream,
    short *samples,
    int preferNewPeriod)
{
    int minPeriod = stream->minPeriod;
    int maxPeriod = stream->maxPeriod;
    int sampleRate = stream->sampleRate;
    int minDiff, maxDiff, retPeriod;
    int skip = 1;
    int period;

    if(sampleRate > SONIC_AMDF_FREQ) {
        skip = sampleRate/SONIC_AMDF_FREQ;
    }
    if(stream->numChannels == 1 && skip == 1) {
        period = findPitchPeriodInRange(samples, minPeriod, maxPeriod, &minDiff, &maxDiff);
    } else {
        downSampleInput(stream, samples, skip);
        period = findPitchPeriodInRange(stream->downSampleBuffer, minPeriod/skip,
            maxPeriod/skip, &minDiff, &maxDiff);
        if(skip != 1) {
            period *= skip;
            minPeriod = period - (skip << 2);
            maxPeriod = period + (skip << 2);
            if(minPeriod < stream->minPeriod) {
                minPeriod = stream->minPeriod;
            }
            if(maxPeriod > stream->maxPeriod) {
                maxPeriod = stream->maxPeriod;
            }
            if(stream->numChannels == 1) {
                period = findPitchPeriodInRange(samples, minPeriod, maxPeriod,
                    &minDiff, &maxDiff);
            } else {
                downSampleInput(stream, samples, 1);
                period = findPitchPeriodInRange(stream->downSampleBuffer, minPeriod,
                    maxPeriod, &minDiff, &maxDiff);
            }
        }
    }
    if(prevPeriodBetter(stream, minDiff, maxDiff, preferNewPeriod)) {
        retPeriod = stream->prevPeriod;
    } else {
        retPeriod = period;
    }
    stream->prevMinDiff = minDiff;
    stream->prevPeriod = period;
    return retPeriod;
}

/* Overlap two sound segments, ramp the volume of one down, while ramping the
   other one from zero up, and add them, storing the result at the output. */
void overlapAdd(
    int numSamples,
    int numChannels,
    short *out,
    short *rampDown,
    short *rampUp)
{
    short *o, *u, *d;
    int i, t;

    for(i = 0; i < numChannels; i++) {
        o = out + i;
        u = rampUp + i;
        d = rampDown + i;
        for(t = 0; t < numSamples; t++) {

            *o = (short) ((*d*(numSamples - t) + *u*t)/numSamples);

            o += numChannels;
            d += numChannels;
            u += numChannels;
        }
    }
}

/* Skip over a pitch period, and copy period/speed samples to the output */
int skipPitchPeriod(
    sonicStream stream,
    short *samples,
    float speed,
    int period)
{
    long newSamples;
    int numChannels = stream->numChannels;

    if(speed >= 2.0f) {
        newSamples = (long) (period/(speed - 1.0f));
    } else {
        newSamples = period;
        stream->remainingInputToCopy = (u32) (period * (2.0f - speed) / (speed - 1.0f));
    }

    enlargeOutputBufferIfNeeded(stream, newSamples);

    overlapAdd(newSamples, numChannels, stream->outputBuffer +
        stream->numOutputSamples*numChannels, samples, samples + period*numChannels);
    stream->numOutputSamples += newSamples;
    return newSamples;
}

/* Insert a pitch period, and determine how much input to copy directly. */
int insertPitchPeriod(
    sonicStream stream,
    short *samples,
    float speed,
    int period)
{
    long newSamples;
    short *out;
    int numChannels = stream->numChannels;

    if(speed < 0.5f) {
        newSamples = (long) (period*speed/(1.0f - speed));
    } else {
        newSamples = period;
        stream->remainingInputToCopy = (u32) (period * (2.0f * speed - 1.0f) / (1.0f - speed));
    }
    enlargeOutputBufferIfNeeded(stream, period + newSamples);

    out = stream->outputBuffer + stream->numOutputSamples*numChannels;
    memcpy(out, samples, period*sizeof(short)*numChannels);
    out = stream->outputBuffer + (stream->numOutputSamples + period)*numChannels;
    overlapAdd(newSamples, numChannels, out, samples + period*numChannels, samples);
    stream->numOutputSamples += period + newSamples;
    return newSamples;
}

/* Resample as many pitch periods as we have buffered on the input.  Return 0 if
   we fail to resize an input or output buffer. */

void changeSpeed(sonicStream stream) {
    float speed = stream->speed;

    short *samples;
    u32 numSamples = stream->numInputSamples;
    int position = 0, period;
    u32 maxRequired = stream->maxRequired;

    /* printf("Changing speed to %f\n", speed); */
    if(stream->numInputSamples < maxRequired) {
        return;
    }

    do
    {
        if(stream->remainingInputToCopy > 0) {
            position += copyInputToOutput(stream, position);;
        }
        else {
            samples = stream->inputBuffer + (position << stream->frame_cnt_to_sample_cnt_shift);
            period = findPitchPeriod(stream, samples, 1);

            if (speed > 1.0f) {
                int newSamples = skipPitchPeriod(stream, samples, speed, period);
                position += period + newSamples;
            } else {
                int newSamples = insertPitchPeriod(stream, samples, speed, period);
                position += newSamples;
            }
        }

    } while(position + maxRequired <= numSamples);
    removeInputSamples(stream, position);
}
