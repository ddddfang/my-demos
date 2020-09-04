#ifndef ALSACAPTUREAUDIO_H
#define ALSACAPTUREAUDIO_H

#include <QtCore>
#include <alsa/asoundlib.h>
#include <signal.h>


#define SAMPLE_FORMAT   16
#define SAMPLE_RATE     44100
#define AUDIO_CHANNEL   2

#define BYTES_PER_FRAME (AUDIO_CHANNEL*(SAMPLE_FORMAT/8))
//60ms 的数据量 = (60/1000)*44100*4 bytes = 2646 * 4 bytes
//2646 看起来好像和 32 除不开,算了,定为2624个frame吧
#define DATASIZE_FRAME   (2624)
#define DATASIZE_BYTES   (DATASIZE_FRAME*BYTES_PER_FRAME)


class audioData {
public:
    audioData():len(0){};
    ~audioData(){
        len = 0;
    }
    uint8_t buf[DATASIZE_BYTES];
    int len;
};


class alsaCaptureAudio
{

public:
    alsaCaptureAudio();
    ~alsaCaptureAudio();
    void readFrames(audioData &adata);   //60ms data

private:
    int init();
    int deinit();
    void capture(audioData &adata, int readframes);

    snd_pcm_t *handle;
    snd_pcm_uframes_t frames;   //period frames
    int size;       //chunk size
};



#endif //ALSACAPTUREAUDIO_H
