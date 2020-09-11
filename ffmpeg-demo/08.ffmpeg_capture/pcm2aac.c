#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/audio_fifo.h>

#ifdef __cplusplus
}
#endif



static int encoding;

void stop_encoding(int param) {
    printf("ctrl+c, stop_encoding\n");
    encoding = 0;
}


int main(int argc, char *argv[])
{
    int ret;
    char *out_filename = "./output.aac";
    char *in_filename = "/home/fang/testvideo/NocturneNo2inEflat_44.1k_s16le.pcm";


    AVFormatContext *pFormatCtx = avformat_alloc_context();
    if (!pFormatCtx) {
        fprintf(stderr, "Could not allocate output format context\n");
        return AVERROR(ENOMEM);
    }

    /* 下面将: 1.创建并绑定 avio ctx, 2.根据文件后缀推测 format, 3.记录url(filename)
       其实这些都是 avformat_open_input 内部做的,但是 ffmpeg 没有提供
       avformat_open_output, 只提供 avformat_alloc_output_context2,
       =av_guess_format + 记录url, avio 还是需要自己打开的 */

    AVIOContext *avio_context = NULL;
    if ((ret = avio_open(&avio_context, out_filename, AVIO_FLAG_WRITE)) < 0) {//avio_open 是特定用于文件的接口,会调用 avio_alloc_context
        fprintf(stderr, "Could not open output file '%s' (error '%s')\n", out_filename, av_err2str(ret));
        return ret;
    }
    pFormatCtx->pb = avio_context;

    if (!(pFormatCtx->oformat = av_guess_format(NULL, out_filename, NULL))) {
        fprintf(stderr, "Could not find output file format\n");
        goto cleanup;
    }

    if (!(pFormatCtx->url = av_strdup(out_filename))) {
        fprintf(stderr, "Could not allocate url.\n");
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 使用上面捏出来的 format ctx 创建一路 stream. */
    AVStream *stream = avformat_new_stream(pFormatCtx, NULL);
    if (!stream) {
        fprintf(stderr, "Could not create new stream\n");
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 输出我们打算使用aac编码,通过 ffmpeg api */
    //AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_MP2);//
    AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);//
    if (!pCodec) {
        fprintf(stderr, "Could not find an AAC encoder.\n");
        goto cleanup;
    }

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        fprintf(stderr, "Could not allocate an encoding context\n");
        ret = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 下一步当然就是为 codec ctx 设置参数了 */
    pCodecCtx->channels = 2;
    pCodecCtx->channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
    pCodecCtx->sample_rate    = pCodec->supported_samplerates ? pCodec->supported_samplerates[0] : 44100;
    pCodecCtx->sample_fmt     = pCodec->sample_fmts ? pCodec->sample_fmts[0] : AV_SAMPLE_FMT_S16;
    //pCodecCtx->bit_rate       = 96000;  //这是控制编码后的码率,设置后通常会采用平均码率的方式, https://blog.csdn.net/owen7500/article/details/51832035

    pCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    /* Set the sample rate for the container. */
    stream->time_base.den = pCodecCtx->sample_rate;
    stream->time_base.num = 1;

    /* Some container formats (like MP4) require global headers to be present.
    * Mark the encoder so that it behaves accordingly. */
    if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        fprintf(stderr, "Could not open output codec (error '%s')\n", av_err2str(ret));
        goto cleanup;
    }

    printf("sample_fmt %d\n", pCodecCtx->sample_fmt);   //1(AV_SAMPLE_FMT_S16)
    printf("frame_size %d\n", pCodecCtx->frame_size);   //4
    printf("sample_rate %d\n", pCodecCtx->sample_rate); //48000
    printf("channel_layout %ld\n", av_get_default_channel_layout(pCodecCtx->channels)); //3(AV_CH_LAYOUT_STEREO)
    printf("bit rate %ld\n", pCodecCtx->bit_rate);


    //从 codec ctx 拷贝信息到 format 的 stream 中
    ret = avcodec_parameters_from_context(stream->codecpar, pCodecCtx);
    if (ret < 0) {
        fprintf(stderr, "Could not initialize stream parameters\n");
        goto cleanup;
    }

    AVFrame *pFrame = av_frame_alloc();     //原始 frame, 鬼知道什么格式
    //根据pcm文件的参数来
    pFrame->format = AV_SAMPLE_FMT_S16;  //AV_SAMPLE_FMT_FLTP
    pFrame->nb_samples = pCodecCtx->frame_size;   //nb_samples 能控制 frame的容量
    pFrame->sample_rate = 44100;
    pFrame->channel_layout = av_get_default_channel_layout(2);//AV_CH_LAYOUT_STEREO;
    av_frame_get_buffer(pFrame, 0);

    AVFrame *pFrameCvt = av_frame_alloc();
    //根据编码器需要的参数来
    pFrameCvt->format = pCodecCtx->sample_fmt;  //AV_SAMPLE_FMT_FLTP
    pFrameCvt->nb_samples = pCodecCtx->frame_size;   //nb_samples 能控制 frame的容量
    pFrameCvt->sample_rate = pCodecCtx->sample_rate;
    pFrameCvt->channel_layout = av_get_default_channel_layout(2);//AV_CH_LAYOUT_STEREO;
    av_frame_get_buffer(pFrameCvt, 0);

    SwrContext *resample_context = swr_alloc_set_opts(  NULL,
        av_get_default_channel_layout(pCodecCtx->channels), pCodecCtx->sample_fmt, pCodecCtx->sample_rate,
        pFrame->channel_layout, pFrame->format, pFrame->sample_rate,
        0, NULL);
    if (!resample_context) {
        fprintf(stderr, "Could not allocate resample context\n");
        return AVERROR(ENOMEM);
    }
    ret = swr_init(resample_context);
    if (ret < 0) {
        fprintf(stderr, "Could not swr_init\n");
        return ret;
    }

    if (signal(SIGINT, stop_encoding) == SIG_ERR) {
        fprintf(stderr, "signal() failed\n");
    }

    FILE *pFile = fopen(in_filename, "rb");
    AVPacket *pkt = av_packet_alloc();
    encoding = 1;

    ret = avformat_write_header(pFormatCtx, NULL);
    while (1) {
get_input:
        if (encoding == 0) {
            break;
        }
        ret = av_frame_make_writable(pFrame);
        if (ret < 0) {
            encoding = 0;
            printf("make writable err.\n");
        }
        int read_size = av_samples_get_buffer_size(NULL, 2, pFrame->nb_samples, pFrame->format, 0);
        if (fread(pFrame->data[0], sizeof(uint8_t), read_size, pFile) != (unsigned)read_size) {
            encoding = 0;
            printf("read err.\n");
        }
        printf("read %d bytes, pFrame->nb_samples %d\n.", read_size, pFrame->nb_samples);

        ret = swr_convert_frame(resample_context, pFrameCvt, pFrame);
        //ret = swr_convert(resample_context, pFrameCvt->data, pFrame->nb_samples, (const uint8_t **)pFrame->data, pFrame->nb_samples);
        if (ret < 0) {
            encoding = 0;
            printf("convert err.\n");
        }
        printf("pFrameCvt->nb_samples %d\n.", pFrameCvt->nb_samples);

decode_frame:
        //int got_frame=0;
        ////Encode
        //ret = avcodec_encode_audio2(pCodecCtx, pkt, pFrameCvt, &got_frame);
        //if(ret < 0){
        //    printf("Failed to encode!\n");
        //    return -1;
        //}
        //if (got_frame==1){
        //    printf("Succeed to encode 1 frame! \tsize:%5d\n", pkt->size);
        //    pkt->stream_index = stream->index;
        //    ret = av_write_frame(pFormatCtx, pkt);
        //    av_packet_unref(pkt);
        //}



        ret = avcodec_send_frame(pCodecCtx, pFrameCvt);
        if (ret < 0) {
            fprintf(stderr, "Error sending the frame to the encoder\n");
            break;
        }

        while (ret >= 0) {
            ret = avcodec_receive_packet(pCodecCtx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                goto get_input;
            }
            else if (ret < 0) {
                fprintf(stderr, "Error encoding audio frame\n");
                return -1;
            }

            printf("got a encoded pkt.pCodecCtx->frame_number %d\n", pCodecCtx->frame_number);
            pkt->stream_index = stream->index;
            //pkt->pts = pCodecCtx->frame_number;
            av_write_frame(pFormatCtx, pkt);
        }

        fflush(stdout);
        usleep(4000);   //400us
    }
    //flush
    ret = avcodec_send_frame(pCodecCtx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error sending the frame to the encoder\n");
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(pCodecCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            fprintf(stderr, "Error again\n");
        }
        else if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
        }
        printf("got a encoded pkt.\n");
        av_write_frame(pFormatCtx, pkt);
    }
    av_write_trailer(pFormatCtx);

cleanup:
    printf("here we quit.\n");

    av_packet_free(&pkt);
    fclose(pFile);
    swr_free(&resample_context);
    av_frame_free(&pFrameCvt);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}
