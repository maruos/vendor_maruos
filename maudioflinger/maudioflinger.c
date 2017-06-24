/*
 * Copyright (C) 2010 The Android Open Source Project
 * Copyright (C) 2017 The Maru OS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Based on Georgian Chituc's ideas and Twaik Yont's prototype for audio in
 * Maru OS. See forum thread for more details:
 * https://groups.google.com/d/msg/maru-os-dev/HzqAdt2bpNw/uPbWlOf5BAAJ
 */

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <cutils/log.h>

#define DEBUG 0

#ifdef ANDROID
#define USE_ANDROID_SIMPLE_BUFFER_QUEUE	 // change to #undef for compatibility testing
#endif
#include <SLES/OpenSLES.h>
#ifdef USE_ANDROID_SIMPLE_BUFFER_QUEUE
#include <SLES/OpenSLES_Android.h>
#endif
#ifdef USE_ANDROID_SIMPLE_BUFFER_QUEUE
#define DATALOCATOR_BUFFERQUEUE SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE
#define IID_BUFFERQUEUE SL_IID_ANDROIDSIMPLEBUFFERQUEUE
#define BufferQueueItf SLAndroidSimpleBufferQueueItf
#define BufferQueueState SLAndroidSimpleBufferQueueState
#define IID_BUFFERQUEUE_USED SL_IID_ANDROIDSIMPLEBUFFERQUEUE
#define INDEX index
#else
#define DATALOCATOR_BUFFERQUEUE SL_DATALOCATOR_BUFFERQUEUE
#define IID_BUFFERQUEUE SL_IID_BUFFERQUEUE
#define BufferQueueItf SLBufferQueueItf
#define BufferQueueState SLBufferQueueState
#define IID_BUFFERQUEUE_USED IID_BUFFERQUEUE
#define INDEX playIndex
#endif
#define checkResult(r) do { \
    if ((r) != SL_RESULT_SUCCESS) { \
        if ((r) == SL_RESULT_PARAMETER_INVALID) ALOGE("error SL_RESULT_PARAMETER_INVALID at %s:%d\n", __FILE__, __LINE__); \
        else if ((r) == SL_RESULT_PRECONDITIONS_VIOLATED ) ALOGE("error SL_RESULT_PRECONDITIONS_VIOLATED at %s:%d\n", __FILE__, __LINE__); \
        else ALOGE("error %d at %s:%d\n", (int) r, __FILE__, __LINE__); \
    } \
    } while (0)

typedef struct {
    short left;
    short right;
} frame_t;

static BufferQueueItf playerBufferqueue;
static struct pollfd pipeFdInfo;

#define MAX_BUFF_SIZE 4096
static frame_t buff[MAX_BUFF_SIZE];

void pipeRead() {
    int ret, len, result;
    ret = poll(&pipeFdInfo, 1, -1);
    if (ret <= 0) {
        ALOGE("error reading audio pipe");
    }

    if (((pipeFdInfo.revents&POLLIN) == POLLIN) || ((pipeFdInfo.revents&POLLPRI) == POLLPRI)) {
        len = read(pipeFdInfo.fd, buff, sizeof(buff));
        if (len > 0) {
            result = (*playerBufferqueue)->Enqueue(playerBufferqueue, buff, len);
        }
    } else if ((pipeFdInfo.revents&POLLHUP) == POLLHUP) {
        ALOGW("Got POLLHUP");
        close(pipeFdInfo.fd);
        pipeFdInfo.fd = -1;
    }
}

// this callback handler is called every time a buffer finishes playing
void bqPlayerCallback(BufferQueueItf bq, void *context){
    pipeRead();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        ALOGE("usage: %s <pipe file path>", argv[0]);
        exit(1);
    }

    SLresult result;

    // create engine
    SLObjectItf engineObject;
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL); checkResult(result);
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE); checkResult(result);
    SLEngineItf engineEngine;
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine); checkResult(result);

    // create output mix
    SLObjectItf outputmixObject;
    result = (*engineEngine)->CreateOutputMix(engineEngine, &outputmixObject, 0, NULL, NULL);  checkResult(result);
    result = (*outputmixObject)->Realize(outputmixObject, SL_BOOLEAN_FALSE); checkResult(result);

    // create audio player
    SLDataSource audiosrc;
    SLDataSink audiosnk;
    SLDataLocator_OutputMix locator_outputmix;
    SLDataLocator_BufferQueue locator_bufferqueue;
    locator_bufferqueue.locatorType = DATALOCATOR_BUFFERQUEUE;
    locator_bufferqueue.numBuffers = 1;
    locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
    locator_outputmix.outputMix = outputmixObject;
    SLDataFormat_PCM pcm;
    pcm.formatType = SL_DATAFORMAT_PCM;
    pcm.numChannels = 2;
    pcm.samplesPerSec = SL_SAMPLINGRATE_44_1;
    pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
    pcm.containerSize = 16;
    pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
    pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
    audiosrc.pLocator = &locator_bufferqueue;
    audiosrc.pFormat = &pcm;
    audiosnk.pLocator = &locator_outputmix;
    audiosnk.pFormat = NULL;
    SLObjectItf playerObject;
    SLInterfaceID ids[1] = {IID_BUFFERQUEUE};
    SLboolean flags[1] = {SL_BOOLEAN_TRUE};
    result = (*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audiosrc, &audiosnk,
            1, ids, flags);  checkResult(result);
    result = (*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE); checkResult(result);
    SLPlayItf playerPlay;
    result = (*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay); checkResult(result);
    result = (*playerObject)->GetInterface(playerObject, IID_BUFFERQUEUE_USED, &playerBufferqueue); checkResult(result);

    // register callback on the buffer queue
    result = (*playerBufferqueue)->RegisterCallback(playerBufferqueue, bqPlayerCallback, NULL); checkResult(result);

    SLuint32 state;
    state = SL_PLAYSTATE_PLAYING;
    result = (*playerPlay)->SetPlayState(playerPlay, state); checkResult(result);

    char pipeFile[128];
    strncpy(pipeFile, argv[1], sizeof(pipeFile)-1);
    pipeFile[sizeof(pipeFile)-1] = '\0';

    ALOGI("Creating audio pipe...");
    // unlink first just in case there is a stale file around
    unlink(pipeFile);
    if (mkfifo(pipeFile, 0666) < 0) {
        ALOGE("Error creating pipe: %s", strerror(errno));
        exit(1);
    }
    // override any existing umask and make sure pipe is 0666
    if (chmod(pipeFile, 0666) < 0) {
        ALOGE("Error chmod'ing pipe: %s", strerror(errno));
        exit(1);
    }

    ALOGI("Waiting on pipe to be opened for write...");
    int fd;
    if((fd = open(pipeFile, O_RDONLY)) == -1) {
        ALOGE("Error opening pipe: %s", strerror(errno));
        exit(1);
    }
    pipeFdInfo.fd = fd;
    pipeFdInfo.events = POLLIN|POLLPRI;


    // When all writers hang up on a pipe, the pipe must be closed and
    // re-opened. We can force the pipe to stay open by hanging on to a dummy
    // writer.
    int dummyWriterFd;
    if ((dummyWriterFd = open(pipeFile, O_WRONLY)) < 0) {
        ALOGE("Error opening pipe for dummy writer: %s", strerror(errno));
        exit(1);
    }

    ALOGI("Reading from pipe...");
    memset(buff, 0, sizeof(buff));
    // enqueue the first sample
    pipeRead();

    while(1){
        // pipeRead called on callback thread
        sleep(60 * 5);
        if (pipeFdInfo.fd < 0) {
            ALOGI("Pipe was closed, exiting.");
            exit(1);
        }
    }
}
