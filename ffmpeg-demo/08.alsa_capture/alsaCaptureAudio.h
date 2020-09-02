#ifndef ALSACAPTUREAUDIO_H
#define ALSACAPTUREAUDIO_H

#include <alsa/asoundlib.h>
#include <signal.h>

class AlsaCaptureAudio
{
public:
    AlsaCaptureAudio();
    ~AlsaCaptureAudio();
    int initAudioCapture();
    void captureAudio();
    void closeCapture();

protected:

    int size;
    snd_pcm_t *handle;
    snd_pcm_uframes_t frames;
    char *buffer;
    FILE *pFile;
};

#endif //ALSACAPTUREAUDIO_H
