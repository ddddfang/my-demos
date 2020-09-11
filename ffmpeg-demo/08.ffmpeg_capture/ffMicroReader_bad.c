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
#include <libavutil/audio_fifo.h>

#ifdef __cplusplus
}
#endif

//OUTPUT_PCM 为 0 输出 aac 编码格式
#define OUTPUT_PCM  0

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


static int open_output( const char *filename,
                        AVFormatContext **output_format_context,    //
                        AVCodecContext **output_codec_context)      //
{
    int error;
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
    if ((error = avio_open(&avio_context, filename, AVIO_FLAG_WRITE)) < 0) {//avio_open 是特定用于文件的接口,会调用 avio_alloc_context
        fprintf(stderr, "Could not open output file '%s' (error '%s')\n", filename, av_err2str(error));
        return error;
    }
    pFormatCtx->pb = avio_context;

    if (!(pFormatCtx->oformat = av_guess_format(NULL, filename, NULL))) {
        fprintf(stderr, "Could not find output file format\n");
        goto cleanup;
    }

    if (!(pFormatCtx->url = av_strdup(filename))) {
        fprintf(stderr, "Could not allocate url.\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 使用上面捏出来的 format ctx 创建一路 stream. */
    AVStream *stream = avformat_new_stream(pFormatCtx, NULL);
    if (!stream) {
        fprintf(stderr, "Could not create new stream\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 输出我们打算使用aac编码,通过 ffmpeg api */
    AVCodec *pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC); //find encoder
    if (!pCodec) {
        fprintf(stderr, "Could not find an AAC encoder.\n");
        goto cleanup;
    }

    AVCodecContext *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!pCodecCtx) {
        fprintf(stderr, "Could not allocate an encoding context\n");
        error = AVERROR(ENOMEM);
        goto cleanup;
    }

    /* 下一步当然就是为 codec ctx 设置参数了 */
    //AVCodec.channel_layouts 是所有支持的 channel_layout
    pCodecCtx->channels = 2;//av_get_channel_layout_nb_channels(pCodec->channel_layouts[0]);
    pCodecCtx->channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
    //编码器本身所有支持的 sample rate 在 AVCodec.supported_samplerates[] 中, doc/examples/encode_audio 选了一个最接近44100的
    pCodecCtx->sample_rate    = pCodec->supported_samplerates ? pCodec->supported_samplerates[0] : 44100;
    //看来编码器本身所有支持的 sample format 的 存放在 AVCodec.sample_fmts[] 中
    pCodecCtx->sample_fmt     = pCodec->sample_fmts ? pCodec->sample_fmts[0] : AV_SAMPLE_FMT_S16;
    //pCodecCtx->bit_rate       = 96000;  //这是控制编码后的码率,设置后通常会采用平均码率的方式, https://blog.csdn.net/owen7500/article/details/51832035

    //Allow the use of the experimental AAC encoder
    pCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

    /* Set the sample rate for the container. */
    stream->time_base.den = pCodecCtx->sample_rate;
    stream->time_base.num = 1;

    /* Some container formats (like MP4) require global headers to be present.
    * Mark the encoder so that it behaves accordingly. */
    if (pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        pCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if ((error = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
        fprintf(stderr, "Could not open output codec (error '%s')\n", av_err2str(error));
        goto cleanup;
    }

    //从 codec ctx 拷贝信息到 format 的 stream 中
    error = avcodec_parameters_from_context(stream->codecpar, pCodecCtx);
    if (error < 0) {
        fprintf(stderr, "Could not initialize stream parameters\n");
        goto cleanup;
    }

    *output_format_context = pFormatCtx;
    *output_codec_context = pCodecCtx;
    return 0;

cleanup:
    avcodec_free_context(&pCodecCtx);
    avio_closep(&pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    *output_format_context = NULL;
    return error < 0 ? error : AVERROR_EXIT;
}

static int close_output(AVFormatContext **out_format_ctx, AVCodecContext **out_codec_ctx) {

    avcodec_close(*out_codec_ctx);
    avcodec_free_context(out_codec_ctx);
    avformat_free_context(*out_format_ctx);
    return 0;
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

    AVFormatContext *pOutFormatCtx = NULL;
    AVCodecContext *pOutCodecCtx = NULL;
    AVPacket *pktOUT = av_packet_alloc();   //用于存放编码了的 aac packet

    open_output("./test.aac", &pOutFormatCtx, &pOutCodecCtx);

    printf("encoder desired: \n\n");
    printf("sample_fmt %d\n", pOutCodecCtx->sample_fmt);   //1(AV_SAMPLE_FMT_S16)
    printf("frame_size %d\n", pOutCodecCtx->frame_size);   //4
    printf("sample_rate %d\n", pOutCodecCtx->sample_rate); //48000
    printf("channel_layout %ld\n", av_get_default_channel_layout(pOutCodecCtx->channels)); //3(AV_CH_LAYOUT_STEREO)

    AVAudioFifo *afifo = av_audio_fifo_alloc(pOutCodecCtx->sample_fmt, pOutCodecCtx->channels, 1);  //先分配可容纳一个 sample的 fifo
    if (!afifo) {
        fprintf(stderr, "Could not allocate FIFO\n");
        return AVERROR(ENOMEM);
    }

    //当需要转码为 aac 的时候,我们就先将拿到的原始 frame 做一个resample,得到适合aac编码的 AVFrame
    //transcode_aac.c是从fifo取出samples后现组装的
    AVFrame *pFrameConverted = av_frame_alloc();  //统一转换

    pFrameConverted->format = pOutCodecCtx->sample_fmt;//AV_SAMPLE_FMT_S16;  //AV_SAMPLE_FMT_FLTP
    pFrameConverted->nb_samples = pOutCodecCtx->frame_size;   //nb_samples 能控制 frame的容量
    pFrameConverted->sample_rate = pOutCodecCtx->sample_rate;
    pFrameConverted->channel_layout = av_get_default_channel_layout(pOutCodecCtx->channels);//AV_CH_LAYOUT_STEREO;
    //不需要对齐,av_samples_get_buffer_size可获得分配了多少字节,理论上为 nb_samples*2(streo)*2(16bit)
    av_frame_get_buffer(pFrameConverted, 0);

    //int out_sample_format = AV_SAMPLE_FMT_S16;
    //int out_nb_samples = 1024;  //pCodecCtx->frame_size
    //int out_sample_rate = 48000;    //44100
    //uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;  //集成式麦克风,立体声,一般是单声道(line-in那种多为 2 channel)
    //int out_nb_channels = av_get_channel_layout_nb_channels(out_channel_layout);
    //int out_buffer_size = av_samples_get_buffer_size(NULL, out_nb_channels, out_nb_samples, out_sample_format, 1);
    //uint8_t *out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE);   //192000 = 1s 48khz 32bit audio
    //avcodec_fill_audio_frame(   pFrameConverted, out_nb_channels, out_sample_format, 
    //                            (const uint8_t *)out_buffer, out_buffer_size, 1);

    struct SwrContext *audio_swr_ctx = swr_alloc();
    if (!audio_swr_ctx) {
        printf("swr_alloc fail.\n");
        return -1;
    }
    //av_opt_set_int(audio_swr_ctx, "in_channel_layout", av_get_default_channel_layout(pCodecCtx->channels), 0);
    //nb_samples 只是代表 AVFrame 中存放的 samples 数量,不需要 swr
    av_opt_set_sample_fmt(audio_swr_ctx, "in_sample_fmt", pCodecCtx->sample_fmt, 0);
    av_opt_set_int(audio_swr_ctx, "in_sample_rate", pCodecCtx->sample_rate, 0);
    av_opt_set_int(audio_swr_ctx, "in_channel_count", pCodecCtx->channels, 0);

    av_opt_set_sample_fmt(audio_swr_ctx, "out_sample_fmt", pOutCodecCtx->sample_fmt, 0);
    av_opt_set_int(audio_swr_ctx, "out_sample_rate", pOutCodecCtx->sample_rate, 0);
    av_opt_set_int(audio_swr_ctx, "out_channel_count", pOutCodecCtx->channels, 0);

    if (swr_init(audio_swr_ctx) < 0) {
        printf("swr_init fail.\n");
        return -1;
    }
#endif
    //int got_picture = 0;
    while (recording == 1) {
        //read packet, and decode(if need), and push to fifo
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
                    //int buffer_size = av_samples_get_buffer_size(NULL, channels, pFrame->nb_samples, pCodecCtx->sample_fmt ,1);
                    //printf("get no.%3d decoded frame, nb_samples %d, channels %d bufsize %d, linesize[0] %d\n", pCodecCtx->frame_number, pFrame->nb_samples, channels, buffer_size, pFrame->linesize[0]);
#if OUTPUT_PCM
                    //printf("%p %p\n", pframe->data[0],pframe->extended_data[0]);    //二者值相等,说明 他们指向了同一块内存
                    fwrite(pFrame->data[0], sizeof(char), pFrame->linesize[0], pFile);
#else
                    if (av_frame_make_writable(pFrameConverted) < 0) {
                        printf ("cannot make frameOUT writable ??\n");
                    }
                    //pFrameConverted->nb_samples 只是决定输出frame的空间,如果比较大的话,可能多次才会填充完整,ret会代表本次resampled的samples数目
                    //nb_samples 都给的 输入 AVFrame 的,这样肯定输出是一致的,输入有多少 samples 我们就转换多少 samples
                    ret = swr_convert(audio_swr_ctx, pFrameConverted->data, pFrame->nb_samples,  //dst
                                (const uint8_t **)pFrame->data, pFrame->nb_samples);    //src

                    //int channels = av_get_channel_layout_nb_channels(pFrameConverted->channel_layout);
                    //printf("get no.%3d resampled frame, nb_samples %d, channels %d, format %d, sample rate %d, ret %d samples, pFrame->nb_samples =%d\n",
                    //        pCodecCtx->frame_number, pFrameConverted->nb_samples, channels, pFrameConverted->format, pFrameConverted->sample_rate, ret, pFrame->nb_samples);

                    //添加 samples 到 fifo. realloc fifo,在原有基础上增加分配
                    printf("fifo size %d, here put %d samples to fifo\n", av_audio_fifo_size(afifo), pFrame->nb_samples);
                    ret = av_audio_fifo_realloc(afifo, av_audio_fifo_size(afifo) + pFrame->nb_samples); //获取的 fifo size 难道是里面有效的数据的长度??
                    if (ret < 0) {
                        fprintf(stderr, "Could not reallocate FIFO\n");
                        recording = 0;
                        break;
                    }
                    if (av_audio_fifo_write(afifo, (void **)pFrameConverted->data, pFrame->nb_samples) < pFrame->nb_samples) {
                        fprintf(stderr, "Could not write data to FIFO\n");
                        recording = 0;
                        break;
                    }
#endif
                    //the picture is allocated by the decoder. no need to free it
                }
            }
        }
#if OUTPUT_PCM
#else
        int fifosize_threhold = pOutCodecCtx->frame_size;   //本codec的能力,一次可以编码的 nb_samples 数目
        if (av_audio_fifo_size(afifo) >= fifosize_threhold) {
            printf("fifo size %d\n", av_audio_fifo_size(afifo));
            AVFrame *pFrame4Dec = av_frame_alloc();  //统一转换

            pFrame4Dec->format = pOutCodecCtx->sample_fmt;
            pFrame4Dec->nb_samples = pOutCodecCtx->frame_size;   //
            pFrame4Dec->sample_rate = pOutCodecCtx->sample_rate;
            pFrame4Dec->channel_layout = pOutCodecCtx->channel_layout;
            av_frame_get_buffer(pFrame4Dec, 0);
            //read samples from fifo here/
            if (av_audio_fifo_read(afifo, (void **)pFrame4Dec->data, pOutCodecCtx->frame_size) < pOutCodecCtx->frame_size) {
                fprintf(stderr, "Could not read data from FIFO\n");
                av_frame_free(&pFrame4Dec);
                return AVERROR_EXIT;
            }
            int channels = av_get_channel_layout_nb_channels(pFrame4Dec->channel_layout);
            printf("nb_samples %d, channels %d, format %d, sample rate %d frame_size=%d\n",
                    pFrame4Dec->nb_samples, channels, pFrame4Dec->format, pFrame4Dec->sample_rate, pOutCodecCtx->frame_size);

            //重新编码为 aac 格式
            ret = avcodec_send_frame(pOutCodecCtx, pFrame4Dec);   // send the frame for encoding
            if (ret < 0) {
                fprintf(stderr, "Error sending the frame to the encoder\n");
                recording = 0;
                break;
            }

            /* read all the available output packets (in general there may be any
             * number of them */
            ret = avcodec_receive_packet(pOutCodecCtx, pktOUT);
            if (ret == AVERROR(EAGAIN)) {
                printf("shit err again.\n");
                return -1;
            }
            if (ret == AVERROR_EOF) {
                printf("shiti eof.\n");
                return -1;
            }
            else if (ret < 0) {
                fprintf(stderr, "Error encoding audio frame\n");
                exit(1);
            }

            if ((ret = av_write_frame(pOutFormatCtx, pktOUT)) < 0) {
                fprintf(stderr, "Could not write frame (error '%s')\n", av_err2str(ret));
                goto cleanup;
            }

            av_packet_unref(pktOUT);

            av_frame_free(&pFrame4Dec);
        }

        //    //int rc = fwrite(buffer, sizeof(char), size, pFile);
        //    //if (rc != size) {
        //    //    fprintf(stderr, "short write: wrote %d bytes\n", rc);
        //    //}
        //
        //}
#endif
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

cleanup:
    printf("here we quit.\n");
#if OUTPUT_PCM
    fclose(pFile);
#else
    swr_free(&audio_swr_ctx);
    av_frame_free(&pFrameConverted);
    if (afifo)
        av_audio_fifo_free(afifo);
    close_output(&pOutFormatCtx, &pOutCodecCtx);
    av_packet_free(&pktOUT);

#endif

    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    av_packet_free(&pkt);
    return 0;
}
