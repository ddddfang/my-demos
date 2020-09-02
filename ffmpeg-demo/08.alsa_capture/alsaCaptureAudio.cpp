#include "alsaCaptureAudio.h"

static int recording;

void stop_recording(int param) {
    recording = 0;
}

AlsaCaptureAudio::AlsaCaptureAudio() {
    pFile = fopen("test.pcm", "wb");
    initAudioCapture();
}
        
AlsaCaptureAudio::~AlsaCaptureAudio() {
    //析构函数自动调用 释放资源
    closeCapture();
}

//初始化
int AlsaCaptureAudio::initAudioCapture() {
    int rc = 0;
    int dir = 0;
    snd_pcm_hw_params_t *params;
    unsigned int sample_rate = 44100;

    //Open PCM device for recording(capture)
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
        return rc;
    }
    printf("open success\n");

    snd_pcm_hw_params_alloca(&params);          //Allocate a hardware parameters object
    snd_pcm_hw_params_any(handle, params);      //Fill it in with default values
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);    //Interleaved mode 交织模式(左右声道信息被交替存在一个frame)
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);    //Signed 16-bit little-endian format 16位小端模式
    snd_pcm_hw_params_set_channels(handle, params, 2);                      //Two channels 双通道
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, &dir);    //44100 bits/sencod sampling rate (CD quality)比特率

    //Set period size to 32 frames, 关于这个参数 https://www.cnblogs.com/lifan3a/articles/4939828.html
    //每个声卡都有一个硬件缓存区(ring buffer)来保存记录下来的样本。当缓存区足够满时，声卡将产生一个中断
    //这里设置当采集32帧数据后,触发一个中断操作, 即 snd_pcm_readi 每次返回理论上应该有 32 帧数据
    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    rc = snd_pcm_hw_params(handle, params);     //write the parameters to the driver
    if (rc < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        return rc;
    }
    printf("write params success\n");

    //Use a buffer large enough to hole one period
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);

    //1 frame = 2 samples(left+right channel), 1 samples = 2 Bytes(16 bit), 所以这里 *4
    size = frames * 4;
    buffer = (char *)malloc(size);

    //这里sample_rate返回的其实是理论上 两次 snd_pcm_readi 返回的时间间隔: (1/44100*32*1000)ms
    //大约七百多微秒
    //snd_pcm_hw_params_get_period_time(params, &sample_rate, &dir);
    return 1;
}

//录制音频
void AlsaCaptureAudio::captureAudio()
{
    int rc = 0;
    recording = 1;
    while (recording)
    {
        //capture data
        rc = snd_pcm_readi(handle, buffer, frames);
        if (rc == -EPIPE) {
            fprintf(stderr, "overrun occurred\n");
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            fprintf(stderr, "error from read: %s\n", snd_strerror(rc));
        } else if (rc != (int)frames) {
            fprintf(stderr, "short read, read %d frames\n", rc);
        }

        printf("write to file......%d\n", size);
        //write to stdout
        rc = fwrite(buffer, sizeof(char), size, pFile);
        if (rc != size) {
            fprintf(stderr, "short write: wrote %d bytes\n", rc);
        }
        if (signal(SIGINT, stop_recording) == SIG_ERR) {
            fprintf(stderr, "signal() failed\n");
        }
    }
}

//释放资源
void AlsaCaptureAudio::closeCapture()
{
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);
    fclose(pFile);
    printf("closeCapture\n");
}

