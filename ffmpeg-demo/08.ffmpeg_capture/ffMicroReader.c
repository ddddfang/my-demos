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
#include <libswresample/swresample.h>

#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
}
#endif

//OUTPUT_PCM 为 0 输出 aac 编码格式
#define OUTPUT_PCM  1

// https://blog.csdn.net/byxdaz/article/details/80713402
//AVCodecContext    中的    frame_size: 对于 ffmpeg 音频codec,表示音频编码的能力,每次只能编码这个数量的采样点数
//AVFrame           中的    nb_samples: 对于 ffmpeg 音频frame,表示每个frame中的采样点数
//一般会设置 AVFrame.nb_samples = AVCodecContext.frame_size
//对于 aac,一般这个值是 1024, 对于 mp3 是 1152

// https://blog.csdn.net/quange_style/article/details/90083173
//对于 audio fmt, 
//若是 AV_SAMPLE_FMT_S16 这类,则 AVFrame 使用 data[0] hold所有通道的交织数据,内部数据 LRLRLRLR.....
//若是 AV_SAMPLE_FMT_S16P 这类 plane类型, AVFrame 使用 data[0] hold LLLLL... 使用 data[1] hold RRRRR... (最多8声道)

#include <signal.h>
static int recording;

void stop_recording(int param) {
    printf("ctrl+c, stop_recording\n");
    recording = 0;
}


static int open_output_file(const char *filename,
                            AVCodecContext *input_codec_context,
                            AVFormatContext **output_format_context,    //
                            AVCodecContext **output_codec_context)      //
{
    int error;

    if (!(*output_format_context = avformat_alloc_context())) {
        fprintf(stderr, "Could not allocate output format context\n");
        return AVERROR(ENOMEM);
    }

    /* 下面将: 1.创建并绑定 avio ctx, 2.根据文件后缀推测 format, 3.记录url(filename)
       其实这些都是 avformat_open_input 内部做的,但是 ffmpeg 没有提供
       avformat_open_output, 只提供 avformat_alloc_output_context2,
       =av_guess_format + 记录url, avio 还是需要自己打开的 */

    AVIOContext *output_io_context = NULL;
    if ((error = avio_open(&output_io_context, filename, AVIO_FLAG_WRITE)) < 0) {//avio_open 是特定用于文件的接口,会调用 avio_alloc_context
        fprintf(stderr, "Could not open output file '%s' (error '%s')\n",
        filename, av_err2str(error));
        return error;
    }
    (*output_format_context)->pb = output_io_context;

    if (!((*output_format_context)->oformat = av_guess_format(NULL, filename, NULL))) {
        fprintf(stderr, "Could not find output file format\n");
        goto cleanup;
    }
    if (!((*output_format_context)->url = av_strdup(filename))) {
        fprintf(stderr, "Could not allocate url.\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 使用上面捏造出来的 format ctx 创建一路 stream. */
    AVStream *stream = avformat_new_stream(*output_format_context, NULL);
    if (!stream) {
        fprintf(stderr, "Could not create new stream\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 输出我们打算使用aac编码,通过 ffmpeg api */
    AVCodec *output_codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!output_codec) {
        fprintf(stderr, "Could not find an AAC encoder.\n");
        goto cleanup;
    }

    AVCodecContext *avctx = avcodec_alloc_context3(output_codec);
    if (!avctx) {
        fprintf(stderr, "Could not allocate an encoding context\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 下一步当然就是为 codec ctx 设置参数了 */
    avctx->channels       = 2;
    avctx->channel_layout = av_get_default_channel_layout(avctx->channels);
    avctx->sample_rate    = input_codec_context->sample_rate;//The input file's sample rate is used to avoid a sample rate conversion.
    avctx->sample_fmt     = output_codec->sample_fmts[0];
    avctx->bit_rate       = 96000;

    //Allow the use of the experimental AAC encoder
    avctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    /* Set the sample rate for the container. */
    stream->time_base.den = input_codec_context->sample_rate;
    stream->time_base.num = 1;

    /* Some container formats (like MP4) require global headers to be present.
    * Mark the encoder so that it behaves accordingly. */
    if ((*output_format_context)->oformat->flags & AVFMT_GLOBALHEADER)
        avctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if ((error = avcodec_open2(avctx, output_codec, NULL)) < 0) {
        fprintf(stderr, "Could not open output codec (error '%s')\n",
        av_err2str(error));
        goto cleanup;
    }

    //从 codec ctx 拷贝信息到 format 的 stream 中
    error = avcodec_parameters_from_context(stream->codecpar, avctx);
    if (error < 0) {
        fprintf(stderr, "Could not initialize stream parameters\n");
        goto cleanup;
    }

    *output_codec_context = avctx;
    return 0;

cleanup:
    avcodec_free_context(&avctx);
    avio_closep(&(*output_format_context)->pb);
    avformat_free_context(*output_format_context);
    *output_format_context = NULL;
    return error < 0 ? error : AVERROR_EXIT;
}

int main(int argc, char *argv[])
{
    int ret;
    char *src_device_name = "default";
    int i, audioindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket *pkt = av_packet_alloc();;

    avdevice_register_all();    //Register Device

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    /* 打开输入流 并查找基本信息 */
    AVInputFormat *ifmt = av_find_input_format("alsa");
    if (avformat_open_input(&pFormatCtx, src_device_name, ifmt, NULL) != 0) {
        printf("Couldn't open input stream.\n");
        return -1;
    }

    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }
    printf("format info:\n");
    av_dump_format(pFormatCtx, 0, src_device_name, 0);

    /* 查找 audio */
    audioindex = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioindex = i;
            break;
        }
    }
    if (audioindex == -1) {
        printf("Couldn't find a audio stream.\n");
        return -1;
    }

    /* 获取 audio codec ctx */
    pCodec = avcodec_find_decoder(pFormatCtx->streams[audioindex]->codecpar->codec_id);
    if (!pCodec) {
        printf("Codec not found.\n");
        return -1;
    }
    pCodecCtx = avcodec_alloc_context3(pCodec); //为 AVCodec 分配一个全新的 AVCodecContext
    if (!pCodecCtx) {
        printf("avcodec_alloc_context3 fail.\n");
        return -1;
    }
    //从流 中拷贝信息到 codec ctx
    if (avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[audioindex]->codecpar) < 0) {
        printf("avcodec_parameters_to_context fail.\n");
        return -1;
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return -1;
    }

    printf("audio codec info:\n");
    printf("sample_fmt %d\n", pCodecCtx->sample_fmt);   //1(AV_SAMPLE_FMT_S16)
    printf("frame_size %d\n", pCodecCtx->frame_size);   //4
    printf("sample_rate %d\n", pCodecCtx->sample_rate); //48000
    printf("channel_layout %ld\n", av_get_default_channel_layout(pCodecCtx->channels)); //3(AV_CH_LAYOUT_STEREO)

    AVFrame *pFrame = av_frame_alloc();     //原始 frame, 鬼知道什么格式

    if (signal(SIGINT, stop_recording) == SIG_ERR) {
        fprintf(stderr, "signal() failed\n");
    }

    recording = 1;

#if OUTPUT_PCM
    FILE *pFile = fopen("test_s16le_ch2_48000hz.pcm", "wb");  //播放的时候就 ffplay test.pcm -f s16le -channels 2 -ar 48000
#else
    //当需要转码为 aac 的时候,我们就先将拿到的原始 frame 做一个resample,得到适合aac编码的 AVFrame
    AVFrame *pFrameOUT = av_frame_alloc();  //统一转换

    pFrameOUT->format = AV_SAMPLE_FMT_S16;
    pFrameOUT->nb_samples = 1024;
    pFrameOUT->sample_rate = 48000;
    pFrameOUT->channel_layout = AV_CH_LAYOUT_STEREO;
    //不需要对齐,av_samples_get_buffer_size可获得分配了多少字节,理论上为 nb_samples*2(streo)*2(16bit)
    //非 plane则 buf[0] 会指向这些数据, linesize[0] 就是 nb_samples*2(streo)*2(16bit)
    av_frame_get_buffer(pFrameOUT, 0);

    //int out_sample_format = AV_SAMPLE_FMT_S16;
    //int out_nb_samples = 1024;  //pCodecCtx->frame_size
    //int out_sample_rate = 48000;    //44100
    //uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;  //集成式麦克风,立体声,一般是单声道(line-in那种多为 2 channel)
    //int out_nb_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    //int out_buffer_size = av_samples_get_buffer_size(NULL, out_nb_channels, out_nb_samples, out_sample_format, 1);
    //uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);   //192000 = 1s 48khz 32bit audio
    //avcodec_fill_audio_frame(   pFrameOUT, out_nb_channels, out_sample_format, 
    //                            (const uint8_t *)out_buffer, out_buffer_size, 1);

    struct SwrContext *audio_swr_ctx = swr_alloc();
    if (!audio_swr_ctx) {
        printf("swr_alloc fail.\n");
        return -1;
    }
    //av_opt_set_int(audio_swr_ctx, "in_channel_layout", av_get_default_channel_layout(pCodecCtx->channels), 0);
    av_opt_set_sample_fmt(audio_swr_ctx, "in_sample_fmt", pCodecCtx->sample_fmt, 0);
    av_opt_set_int(audio_swr_ctx, "in_sample_rate", pCodecCtx->sample_rate, 0);
    av_opt_set_int(audio_swr_ctx, "in_channel_count", pCodecCtx->channels, 0);

    av_opt_set_sample_fmt(audio_swr_ctx, "out_sample_fmt", pFrameOUT->format, 0);
    av_opt_set_int(audio_swr_ctx, "out_sample_rate", pFrameOUT->sample_rate, 0);
    av_opt_set_int(audio_swr_ctx, "out_channel_count", av_get_channel_layout_nb_channels(pFrameOUT->channel_layout), 0);

    if (swr_init(audio_swr_ctx) < 0) {
        printf("swr_init fail.\n");
        return -1;
    }
#endif
    //int got_picture = 0;
    while (recording == 1) {
        if (av_read_frame(pFormatCtx, pkt) >= 0) {
            if (pkt->stream_index == audioindex) {
                //最新的decode方法
                ret = avcodec_send_packet(pCodecCtx, pkt);
                if (ret < 0) {
                    fprintf(stderr, "Error sending a packet for decoding\n");
                    return -1;
                }

                while (ret >= 0) {
                    ret = avcodec_receive_frame(pCodecCtx, pFrame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        recording = 0;
                        break;
                    }
                    //int channels = av_get_channel_layout_nb_channels(pFrameOUT->channel_layout);
                    //int buffer_size = av_samples_get_buffer_size(NULL, channels, pFrame->nb_samples, pCodecCtx->sample_fmt ,1);
                    //printf("get no.%3d decoded frame, nb_samples %d, channels %d bufsize %d, linesize[0] %d\n", pCodecCtx->frame_number, pFrame->nb_samples, channels, buffer_size, pFrame->linesize[0]);
#if OUTPUT_PCM
                    //printf("%p %p\n", pframe->data[0],pframe->extended_data[0]);    //二者值相等,说明 他们指向了同一块内存
                    fwrite(pFrame->data[0], sizeof(char), pFrame->linesize[0], pFile);
#else
                    //swr_convert(audio_swr_ctx, pFrameOUT->data, pFrameOUT->nb_samples,  //dst
                    //            (const uint8_t **)pFrame->data, pFrame->nb_samples);    //src
                    //int rc = fwrite(buffer, sizeof(char), size, pFile);
                    //if (rc != size) {
                    //    fprintf(stderr, "short write: wrote %d bytes\n", rc);
                    //}
#endif
                    //the picture is allocated by the decoder. no need to free it

                }
            }
        }
        usleep(400);   //400us
    }
    //flush decoder
    pkt->data = NULL;
    pkt->size = 0;
    ret = avcodec_send_packet(pCodecCtx, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error sending a packet for decoding\n");
        return -1;
    }
    while (ret >= 0) {
        ret = avcodec_receive_frame(pCodecCtx, pFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0) {
            fprintf(stderr, "Error during decoding\n");
            break;
        }

        printf("saving frame %3d\n", pCodecCtx->frame_number);

#if OUTPUT_PCM
        fwrite(pFrame->data[0], sizeof(char), pFrame->linesize[0], pFile);
#else
#endif
    }

    printf("here we quit.\n");
#if OUTPUT_PCM
    fclose(pFile);
#else
    swr_free(&audio_swr_ctx);
    av_frame_free(&pFrameOUT);
#endif

    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    av_packet_free(&pkt);
    return 0;
}
