#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <binder/IPCThreadState.h>
#include <media/ICrypto.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaCodec.h>
#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <ui/DisplayInfo.h>

using namespace android;

#define VDEV_BITRATE    1000000
#define VDEV_FRATERATE  25

int main(void)
{
    sp<ProcessState> self = ProcessState::self();
    self->startThreadPool();

    sp<IBinder> mdisp = SurfaceComposerClient::getBuiltInDisplay(ISurfaceComposer::eDisplayIdMain);
    DisplayInfo maindispinfo;
    SurfaceComposerClient::getDisplayInfo(mdisp, &maindispinfo);
    printf("maindispinfo.w: %d\n", maindispinfo.w);
    printf("maindispinfo.h: %d\n", maindispinfo.h);
    printf("maindispinfo.orientation: %d\n", maindispinfo.orientation);

    sp<AMessage> format = new AMessage;
    format->setInt32 ("width"           , maindispinfo.w);
    format->setInt32 ("height"          , maindispinfo.h);
    format->setString("mime"           , "video/avc"    );
    format->setInt32 ("color-format"    , 0x7F000789    );
    format->setInt32 ("bitrate"         , VDEV_BITRATE  );
    format->setFloat ("frame-rate"      , VDEV_FRATERATE);
    format->setInt32 ("i-frame-interval", 5             );

    sp<ALooper> looper = new ALooper;
    looper->setName("libscreenrecord_looper");
    looper->start();

    sp<MediaCodec> codec = MediaCodec::CreateByType(looper, "video/avc", true);
    codec->configure(format, NULL, NULL, MediaCodec::CONFIGURE_FLAG_ENCODE);

    sp<IGraphicBufferProducer> bufferProducer;
    codec->createInputSurface(&bufferProducer);
    codec->start();

    Rect srcrect(0, 0, maindispinfo.w, maindispinfo.h), dstrect(0, 0, (maindispinfo.w + 31) & (~31), maindispinfo.h);
    sp<IBinder> vdisp = SurfaceComposerClient::createDisplay(String8("vdev_test"), false);
    SurfaceComposerClient::openGlobalTransaction();
    SurfaceComposerClient::setDisplaySurface(vdisp, bufferProducer);
    SurfaceComposerClient::setDisplayProjection(vdisp, DISPLAY_ORIENTATION_0, srcrect, dstrect);
    SurfaceComposerClient::setDisplayLayerStack(vdisp, 0);
    SurfaceComposerClient::closeGlobalTransaction();

    Vector<sp<ABuffer>> buffers;
    codec->getOutputBuffers(&buffers);

    while (1) {
        size_t   bufIndex, offset, size;
        int64_t  ptsUsec = 0;
        uint32_t flags;
        status_t err = codec->dequeueOutputBuffer(&bufIndex, &offset, &size, &ptsUsec, &flags, 100*1000);
        switch (err) {
        case NO_ERROR:
            printf("get h264 data ! bufIndex: %d, offset: %d, size: %d, flags: %x, data: %p\n", (int)bufIndex, (int)offset, (int)size, flags, buffers[bufIndex]->data());
            if (ptsUsec == 0) ptsUsec = systemTime(SYSTEM_TIME_MONOTONIC) / 1000;
            codec->releaseOutputBuffer(bufIndex);
            break;
        case -EAGAIN:
            printf("got EAGAIN !\n");
            break;
        case INFO_FORMAT_CHANGED:
            printf("INFO_FORMAT_CHANGED !\n");
            break;
        case INFO_OUTPUT_BUFFERS_CHANGED:
            printf("INFO_OUTPUT_BUFFERS_CHANGED !\n");
            codec->getOutputBuffers(&buffers);
            break;
        case INVALID_OPERATION:
            printf("INVALID_OPERATION !\n");
            break;
        }
    }

    SurfaceComposerClient::destroyDisplay(vdisp);
    codec->stop();
    codec->release();
    return 0;
}

