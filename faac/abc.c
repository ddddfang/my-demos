#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <faac.h>

int main(int argc, char *argv[])
{
    unsigned long nSampleRate = 44100;  // 采样率
    unsigned int nChannels = 2;         // 声道数
    unsigned int nPCMBitSize = 16;      // 单样本位数

    int nRet;

    FILE *fpIn = fopen("/home/fang/testvideo/NocturneNo2inEflat_44.1k_s16le.pcm", "rb");
    FILE *fpOut = fopen("./out.aac", "wb");

    unsigned long nInputSamples = 0;    //这类似ffmpeg 中的 frame_size
    unsigned long nMaxOutputBytes = 0;
    // (1) Open FAAC engine
    faacEncHandle hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    if(hEncoder == NULL) {
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    } else {
        printf("open success, nInputSamples %ld, nMaxOutputBytes %ld\n", nInputSamples, nMaxOutputBytes);
    }

    //int nPCMBufferSize = nInputSamples * (nPCMBitSize / 8) * nChannels;
    int nPCMBufferSize = nInputSamples * (nPCMBitSize / 8); //这里的理解和 ffmpeg 有差异,看起来不能加上 channels
    uint8_t *pbPCMBuffer = (uint8_t *)malloc(nPCMBufferSize * sizeof(uint8_t));
    uint8_t *pbAACBuffer = (uint8_t *)malloc(nMaxOutputBytes * sizeof(uint8_t));

    // (2.1) Get current encoding configuration
    faacEncConfigurationPtr pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
    pConfiguration->inputFormat = FAAC_INPUT_16BIT;

    // (2.2) Set encoding configuration
    nRet = faacEncSetConfiguration(hEncoder, pConfiguration);

    for(int i = 0; 1; i++) {
        // 读入的实际字节数，最大不会超过nPCMBufferSize，一般只有读到文件尾时才不是这个值
        int nBytesRead = fread(pbPCMBuffer, 1, nPCMBufferSize, fpIn);

        // 输入样本数，用实际读入字节数计算，一般只有读到文件尾时才不是nPCMBufferSize/(nPCMBitSize/8);
        //int nSamples = nBytesRead / (nPCMBitSize / 8) / nChannels;
        int nSamples = nBytesRead / (nPCMBitSize / 8);

        // (3) Encode,返回的是编码后的字节数
        nRet = faacEncEncode(hEncoder, (int *) pbPCMBuffer, nSamples, pbAACBuffer, nMaxOutputBytes);
        printf("%d: faacEncEncode returns %d\n", i, nRet);

        fwrite(pbAACBuffer, 1, nRet, fpOut);

        if(nBytesRead <= 0) {
            break;
        }
    }

    // (4) Close FAAC engine
    nRet = faacEncClose(hEncoder);

    free(pbPCMBuffer);
    free(pbAACBuffer);
    fclose(fpOut);
    fclose(fpIn);

    return 0;
}
