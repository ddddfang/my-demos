#include "alsaCaptureAudio.h"
#include <QTime>
#include <iostream>


alsaCaptureAudio::alsaCaptureAudio() {
    std::cout << "construct alsaCaptureAudio." << std::endl;
    this->init();
}

alsaCaptureAudio::~alsaCaptureAudio() {
    std::cout << "destruct alsaCaptureAudio." << std::endl;
    this->deinit();
}

void alsaCaptureAudio::readFrames(audioData &adata) {
    int want_frames = DATASIZE_FRAME;
    this->capture(adata, want_frames);
}

//--------------
int alsaCaptureAudio::init() {
    int rc = 0;
    int dir = 0;
    snd_pcm_hw_params_t *params;
    unsigned int sample_rate = SAMPLE_RATE;

    //Open PCM device for recording(capture), "default" is device name
    rc = snd_pcm_open(&(this->handle), "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        std::cout << "unable to open pcm device: " << snd_strerror(rc) << std::endl;
        return rc;
    }
    std::cout << "open success." << std::endl;

    snd_pcm_hw_params_alloca(&params);          //Allocate a hardware parameters object
    snd_pcm_hw_params_any(handle, params);      //Fill it in with default values
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);    //Interleaved mode 交织模式(左右声道信息被交替存在一个frame)
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);    //Signed 16-bit little-endian format 16位小端模式
    snd_pcm_hw_params_set_channels(handle, params, AUDIO_CHANNEL);//Two channels 双通道
    snd_pcm_hw_params_set_rate_near(handle, params, &sample_rate, &dir);    //44100 bits/sencod sampling rate (CD quality)比特率

    //Set period size to 32 frames, 关于这个参数 https://www.cnblogs.com/lifan3a/articles/4939828.html
    //每个声卡都有一个硬件缓存区(ring buffer)来保存记录下来的样本。当缓存区足够满时，声卡将产生一个中断
    //这里设置当采集32帧数据后,触发一个中断操作, 即 snd_pcm_readi 每次返回理论上应该有 32 帧数据
    this->frames = 32;
    snd_pcm_hw_params_set_period_size_near(this->handle, params, &(this->frames), &dir);

    rc = snd_pcm_hw_params(this->handle, params);     //write the parameters to the driver
    if (rc < 0) {
        std::cout << "unable to set hw parameters: " << snd_strerror(rc) << std::endl;
        return rc;
    }
    std::cout << "write params success." << std::endl;

    //Use a buffer large enough to hole one period
    snd_pcm_hw_params_get_period_size(params, &this->frames, &dir);

    //1 frame = 2 samples(left+right channel), 1 samples = 2 Bytes(16 bit), 所以这里 *4
    this->size = this->frames * BYTES_PER_FRAME;  //chunk size

    //这里sample_rate返回的其实是理论上 两次 snd_pcm_readi 返回的时间间隔: (1/44100*32*1000)ms
    //大约七百多微秒
    //snd_pcm_hw_params_get_period_time(params, &sample_rate, &dir);
    return 1;
}

int alsaCaptureAudio::deinit() {
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    printf("closeCapture\n");
    return 1;
}

void alsaCaptureAudio::capture(audioData &adata, int readframes) {
    int rc = 0;
    int chunk_frames = (int)this->frames;
    int res_frames = readframes;
    uint8_t *pbuf = adata.buf;
    adata.len = 0;

    while (res_frames > 0) {
        if (res_frames < chunk_frames) {    // >=32 frames,尾巴不要了
            break;
        }
        int cur_read_frames = (res_frames > chunk_frames) ? chunk_frames : res_frames;

        //capture data
        rc = snd_pcm_readi(this->handle, pbuf, cur_read_frames); //一次读取 frames 帧, 即一个 chunk size
        if (rc == -EAGAIN || (rc > 0 && rc < (int)cur_read_frames)) {
            std::cout << "rc: " << rc << std::endl;
            snd_pcm_wait(this->handle, 1000);
        } else if (rc == -EPIPE) {
            std::cout << "overrun occurred !" << std::endl;
            snd_pcm_prepare(this->handle);
        } else if (rc < 0) {
            std::cout << "error when read: " << snd_strerror(rc) << std::endl;
        }
        if (rc > 0) {
            pbuf += rc * BYTES_PER_FRAME;
            adata.len += rc * BYTES_PER_FRAME;
            res_frames -= rc;
        }
    }
}


//#include <sys/time.h>  
//
////录制音频
//void AlsaCaptureAudio::captureAudio()
//{
//    struct timeval tpstart,tpend;
//    float timeusems;
//
//    int rc = 0;
//    recording = 1;
//    while (recording)
//    {
//        gettimeofday(&tpstart, NULL);
//        
//        //capture data
//        rc = snd_pcm_readi(handle, buffer, frames);
//
//        gettimeofday(&tpend,NULL);
//        timeusems = (1000000*(tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec) / 1000.0;
//        printf("%f ms elapsed.\n", timeusems);
//
//        //QThread::usleep(1000/fps);
//
//        if (rc == -EAGAIN || (rc > 0 && rc < (int)frames)) {
//            std::cout << "rc: " << rc << std::endl;
//            snd_pcm_wait(handle, 1000);
//        } else if (rc == -EPIPE) {
//            std::cout << "overrun occurred !" << std::endl;
//            snd_pcm_prepare(handle);
//        } else if (rc < 0) {
//            std::cout << "error when read: " << snd_strerror(rc) << std::endl;
//        }
//
//        printf("write to file......%d\n", size);
//        //write to stdout
//        rc = fwrite(buffer, sizeof(char), size, pFile);
//        if (rc != size) {
//            fprintf(stderr, "short write: wrote %d bytes\n", rc);
//        }
//        if (signal(SIGINT, stop_recording) == SIG_ERR) {
//            fprintf(stderr, "signal() failed\n");
//        }
//    }
//}
//
