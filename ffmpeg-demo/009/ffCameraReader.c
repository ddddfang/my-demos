#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>

#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

//ffmpeg -devices 列出所有可用的 devices

#include <signal.h>
static int recording;

void stop_recording(int param) {
    recording = 0;
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    int i;

    f = fopen(filename,"w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}


int main(int argc, char *argv[])
{
    char *src_device_name = "/dev/video1";
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket *pkt = av_packet_alloc();;

    //av_register_all();        //现在已经不用了,直接 find_format, formats demuxs 都是静态注册的全局变量
    avformat_network_init();    //?
    avdevice_register_all();    //Register Device

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //// DSHOW
    //AVInputFormat *ifmt=av_find_input_format("dshow");
    ////Set own video device's name
    //if(avformat_open_input(&pFormatCtx,"video=Integrated Camera",ifmt,NULL)!=0){
    //    printf("Couldn't open input stream.\n");
    //    return -1;
    //}
    //// MAC
    //show_avfoundation_device();
    ////Mac
    //AVInputFormat *ifmt=av_find_input_format("avfoundation");
    ////Avfoundation
    ////[video]:[audio]
    //if(avformat_open_input(&pFormatCtx,"0",ifmt,NULL)!=0){
    //    printf("Couldn't open input stream.\n");
    //    return -1;
    //}
    // LINUX
    AVInputFormat *ifmt = av_find_input_format("video4linux2");
    if (avformat_open_input(&pFormatCtx, src_device_name, ifmt, NULL) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    av_dump_format(pFormatCtx, 0, src_device_name, 0);

    videoindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        //if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoindex = i;
            break;
        }
    }
    if (videoindex == -1) {
        printf("Couldn't find a video stream.\n");
        return -1;
    }

    //原来获取这个stream的 codec 信息是这样的:
    //pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    //pCodecCtx = pFormatCtx->streams[videoindex]->codec;   //主要是这一步

    //现在不推荐上面那样,获取这个stream的 codec 信息是这样的:
    pCodec = avcodec_find_decoder(pFormatCtx->streams[videoindex]->codecpar->codec_id);
    if (!pCodec) {
        printf("Codec not found.\n");
        return -1;
    }

    //为 AVCodec 分配一个全新的 AVCodecContext
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        printf("avcodec_alloc_context3 fail.\n");
        return -1;
    }
    //拷贝 原 stream 中的 codecpar 信息
    if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar) < 0) {
        printf("avcodec_parameters_to_context fail.\n");
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    AVFrame *pFrame = av_frame_alloc();     //原始 frame, 鬼知道什么格式
    AVFrame *pFrameYUV = av_frame_alloc();  //统一转换为 yuv
    //av_frame_alloc只是分配了一个壳,原始 frame 在decode出来的时候 ffmpeg会为其分配空间,但这里的 frame可就需要我们自己来了
    unsigned char *frame_yuv_buf = av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize,
                        frame_yuv_buf, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
    //pFrameYUV->format = AV_PIX_FMT_YUV420P;
    //pFrameYUV->width = pCodecCtx->width;
    //pFrameYUV->height = pCodecCtx->height;
    //av_frame_get_buffer(pFrameYUV, 32);   //这应该与上面是等价的,ffmpeg/doc/examples/muxing.c 里面就是这样分配的

    struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                                        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
                                                        SWS_BICUBIC, NULL, NULL, NULL); 

    if (signal(SIGINT, stop_recording) == SIG_ERR) {
        fprintf(stderr, "signal() failed\n");
    }

    recording = 1;
    //int got_picture = 0;
    int ret;
    char filename_buf[1024];
    while (recording == 1) {
        if (av_read_frame(pFormatCtx, pkt) >= 0) {
            if (pkt->stream_index == videoindex) {
                //if (avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, pkt) < 0) {//decode得到的就是yuv,ffmpeg固定流程
                //    printf("Decode Error.\n");
                //    return -1;
                //}
                //if (got_picture) {
                //    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize,
                //                0, pCodecCtx->height,
                //                pFrameYUV->data, pFrameYUV->linesize);
                //}

                ////最新的decode方法
                //ret = avcodec_send_packet(pCodecCtx, pkt);
                //if (ret < 0) {
                //    fprintf(stderr, "Error sending a packet for decoding\n");
                //    return -1;
                //}

                while (ret >= 0) {
                    ret = avcodec_receive_frame(pCodecCtx, pFrame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        return -1;
                    }
                    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize,
                                0, pCodecCtx->height,
                                pFrameYUV->data, pFrameYUV->linesize);
                    //the picture is allocated by the decoder. no need to free it


                    printf("saving frame %3d\n", pCodecCtx->frame_number);
                    fflush(stdout);
                    snprintf(filename_buf, sizeof(filename_buf), "%s-%d", "decoded", pCodecCtx->frame_number);
                    pgm_save(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, filename_buf);
                }
            }
        }
        usleep(40000);   //40ms
    }

    sws_freeContext(img_convert_ctx);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    av_packet_free(&pkt);
    return 0;
}
