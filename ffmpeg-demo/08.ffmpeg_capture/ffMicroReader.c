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
#endif
    }

    printf("here we quit.\n");
#if OUTPUT_PCM
    fclose(pFile);
#endif

    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avcodec_free_context(&pCodecCtx);
    avformat_close_input(&pFormatCtx);
    av_packet_free(&pkt);
    return 0;
}
