#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <media/AudioRecord.h>

using namespace android;

int main(void)
{
    size_t               minFrameCount  = 0;
    int                  sampleRateInHz = 8000;
    audio_format_t       audioFormat    = AUDIO_FORMAT_PCM_16_BIT;
    audio_channel_mask_t channelConfig  = AUDIO_CHANNEL_IN_MONO;
    audio_source_t       inputSource    = AUDIO_SOURCE_MIC;
    sp<AudioRecord>      pAudioRecord   = new AudioRecord(String16("adev_test"));
    uint8_t             *inBuffer       = NULL;
    int                  inBufSize      = 0;

    if (pAudioRecord == NULL) {
        printf("create native AudioRecord failed !\n");
        goto done;
    }
    if (pAudioRecord->set(inputSource, sampleRateInHz, audioFormat, channelConfig) != NO_ERROR) {
        printf("pAudioRecord->set() failed !\n");
        goto done;
    }
    if (pAudioRecord->initCheck() != NO_ERROR) {
        printf("pAudioRecord->initCheck() failed !\n");
        goto done;
    }
    if (pAudioRecord->start() != NO_ERROR) {
        printf("pAudioRecord->start() failed !\n");
        goto done;
    }

    if (AudioRecord::getMinFrameCount(&minFrameCount, sampleRateInHz, audioFormat, channelConfig) != NO_ERROR) {
        printf("AudioRecord::getMinFrameCount failed !\n");
        goto done;
    }
    inBufSize = (channelConfig == AUDIO_CHANNEL_IN_MONO ? 1 : 2) * sizeof(int16_t) * minFrameCount;
    inBuffer  = (uint8_t*)malloc(inBufSize);
    if (!inBuffer) {
        printf("failed to allocate inBuffer !\n");
        goto done;
    }

    while (1) {
        int readlen = pAudioRecord->read(inBuffer, inBufSize);
        printf("pAudioRecord->read readlen: %d\n", readlen);
    }

done:
    if (inBuffer) free(inBuffer);
    if (pAudioRecord != NULL) pAudioRecord->stop();
    return 0;
}

