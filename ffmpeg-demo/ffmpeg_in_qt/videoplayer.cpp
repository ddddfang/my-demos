#include "videoplayer.h"
#include <QDebug>
#include <SDL2/SDL.h>

videoPlayer::videoPlayer():mStop(false),mFilePath("")
{
    cout << "videoPlayer constructed: threadid=" << QThread::currentThreadId()<<endl;
}

videoPlayer::~videoPlayer() //when you close window
{
    cout << "videoPlayer decstructed: threadid=" << QThread::currentThreadId()<<endl;
}

void videoPlayer::stop()
{
    qDebug()<<"myThread::stop call. id="<<currentThreadId();
    mMutex.lock();
    mStop = true;
    mMutex.unlock();
}

void videoPlayer::setFilePath(QString path)
{
    mFilePath = path;
}

bool videoPlayer::isRealtime(AVFormatContext *pFormatCtx)
{
    if (!strcmp(pFormatCtx->iformat->name, "rtp")
        || !strcmp(pFormatCtx->iformat->name, "rtsp")
        || !strcmp(pFormatCtx->iformat->name, "sdp")) {
         return true;
    }

    if(pFormatCtx->pb && (!strncmp(pFormatCtx->filename, "rtp:", 4)
        || !strncmp(pFormatCtx->filename, "udp:", 4)
        )) {
        return true;
    }

    return false;
}

void videoPlayer::run()
{
    cout << "videoPlayer run: threadid=" << QThread::currentThreadId()<<endl;
    // Initalizing these to NULL prevents segfaults!
    AVFormatContext *pFormatCtx = NULL;
    int i, videoStream;
    AVCodecContext *pCodecCtxOrig = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVCodec *pCodec = NULL;
    AVFrame *pFrame = NULL;
    AVFrame *pFrameRGB = NULL;
    AVPacket packet;
    int frameFinished;
    int numBytes;
    uint8_t *buffer = NULL;
    struct SwsContext *sws_ctx = NULL;
    bool realTime = true;

    if(mFilePath.isEmpty() || mFilePath.isNull())
    {
        cout<<"no valid file to play" <<endl;
        goto EXIT;
    }
    cout<<"we are going to play: "<<mFilePath.toStdString()<<endl;

    avfilter_register_all();    // filters
    av_register_all();          // register all formats and codecs
    avformat_network_init();    // ffmpeg network init for rtsp
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {    // sdl init
        cout << "SDL init failed" <<endl;
    }

    if(avformat_open_input(&pFormatCtx, mFilePath.toLocal8Bit().data(), NULL, NULL)!=0)
    {
        cout << "avformat_open_input fail."<<endl;
        goto EXIT;
    }

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    {
        cout << "avformat_find_stream_info fail."<<endl;
        goto EXIT;
    }
    realTime = isRealtime(pFormatCtx);
    if (!realTime) {
        emit videoTimeOK(pFormatCtx->duration); //after avformat_find_stream_info()
        cout<<"timetotal is "<<pFormatCtx->duration<<endl;
    } else {
        emit videoTimeOK(0);
    }

    av_dump_format(pFormatCtx, 0, mFilePath.toLocal8Bit().data(), 0);

    videoStream = -1;
    for(i=0; i<pFormatCtx->nb_streams; i++){
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if(videoStream == -1)
    {
        cout << "avformat_find_stream_info fail. no video stream."<<endl;
        goto EXIT;
    }

    // AVFormatContext --> AVCodecContext(origin, alloc auto when avformat_open_input)
    pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;

    // AVCodecContext --> codec_id --> AVCodec(global)
    pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if(pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        goto EXIT;
    }

    // AVCodec -->	AVCodecContext(a new)
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
        fprintf(stderr, "Couldn't copy codec context");
        goto EXIT;
    }

    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
    {
        cout << "avcodec_open2 fail."<<endl;
        goto EXIT;
    }

    // fang: "uint8_t *data[AV_NUM_DATA_POINTERS]" have not alloc yet,alloc by avcodec_decode_video2()?
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    if(pFrameRGB == NULL)
    {
        cout << "av_frame_alloc pFrameRGB fail."<<endl;
        goto EXIT;
    }

    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    // fullfil "uint8_t *data[AV_NUM_DATA_POINTERS]" and "int linesize[AV_NUM_DATA_POINTERS]" in AVPicture,
    // AVFrame is a superset of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB32, pCodecCtx->width, pCodecCtx->height);

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(	pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGB32,
                                SWS_BILINEAR, NULL, NULL, NULL);

    // Read frames and save first five frames to disk
    i=0;
    mMutex.lock();
    mStop = false;
    mMutex.unlock();
    while(true) {
        mMutex.lock();
        if(mStop)
        {
            mMutex.unlock();
            break;
        }
        mMutex.unlock();

        if(av_read_frame(pFormatCtx, &packet) < 0)
        {
            cout << "av_read_frame < 0,reached file end."<<endl;
            break;
        }
        // Is this a packet from the video stream?
        if(packet.stream_index == videoStream) {
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);	//

            // Did we get a video frame?
            if(frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale(	sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                            pFrameRGB->data, pFrameRGB->linesize);

                //send the image to main thread
                QImage tmpImg((uchar *)buffer,pCodecCtx->width,pCodecCtx->height,QImage::Format_RGB32);
                QImage image = tmpImg.copy();
                emit processOK(image);
            }
        }
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
EXIT:
    // Free the RGB image
    if(buffer)
        av_free(buffer);
    if(pFrameRGB)
        av_frame_free(&pFrameRGB);

    // Free the YUV frame
    if(pFrame)
       av_frame_free(&pFrame);

    // Close the codecs
    if(pCodecCtx)
        avcodec_close(pCodecCtx);
    if(pCodecCtxOrig)
        avcodec_close(pCodecCtxOrig);

    // Close the video file
    if(pFormatCtx)
        avformat_close_input(&pFormatCtx);
    cout << "videoPlayer exit: threadid=" << QThread::currentThreadId()<<endl;
    return;
}

