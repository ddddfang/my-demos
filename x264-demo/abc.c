#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if defined ( __cplusplus)
extern "C"
{
    #include "x264.h"
};

#else

#include "x264.h"

#endif

#define FAIL_IF_ERROR( cond, ... )\
do\
{\
    if( cond )\
    {\
        fprintf( stderr, __VA_ARGS__ );\
        goto fail;\
    }\
} while( 0 )

//使用的时候
// cat /home/fang/testvideo/sintel_640_360_yuv420p.yuv | ./abc 640x360 > sintel.h264
//或者
// ./abc 640x360 </home/fang/testvideo/sintel_640_360_yuv420p.yuv >sintel.h264 //<表示输入, >重定向输出
int main(int argc, char** argv)
{
    int width, height;
    x264_param_t param;
    x264_picture_t pic;
    x264_picture_t pic_out;
    x264_t *h;
    int i_frame = 0;
    int i_frame_size;
    x264_nal_t *nal;
    int i_nal;

#ifdef _WIN32
    _setmode( _fileno( stdin ),  _O_BINARY );
    _setmode( _fileno( stdout ), _O_BINARY );
    _setmode( _fileno( stderr ), _O_BINARY );
#endif

    FAIL_IF_ERROR( !(argc > 1), "Example usage: example 352x288 <input.yuv >output.h264\n" );
    FAIL_IF_ERROR( 2 != sscanf( argv[1], "%dx%d", &width, &height ), "resolution not specified or incorrect\n" );

    /* Get default params for preset/tuning */
    if( x264_param_default_preset( &param, "medium", NULL ) < 0 )
        goto fail;

    /* Configure non-default params */
    param.i_log_level = X264_LOG_DEBUG;
    param.i_bitdepth = 8;
    param.i_csp = X264_CSP_I420;
    param.i_width  = width;
    param.i_height = height;
    param.b_vfr_input = 0;
    param.b_repeat_headers = 1;
    param.b_annexb = 1;
//    x264_param_default(pParam);
//    pParam->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
//    pParam->i_frame_total = 0;
//    pParam->i_keyint_max = 10;
//    pParam->i_bframe = 5;
//    pParam->b_open_gop = 0;
//    pParam->i_bframe_pyramid = 0;
//    pParam->rc.i_qp_constant = 0;
//    pParam->rc.i_qp_max = 0;
//    pParam->rc.i_qp_min = 0;
//    pParam->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
//    pParam->i_fps_den = 1;
//    pParam->i_fps_num = 25;
//    pParam->i_timebase_den = pParam->i_fps_num;
//    pParam->i_timebase_num = pParam->i_fps_den;
//    x264_param_apply_profile(pParam, x264_profile_names[5]);

    /* Apply profile restrictions. */
    if( x264_param_apply_profile( &param, "high" ) < 0 )
        goto fail;

    if( x264_picture_alloc( &pic, param.i_csp, param.i_width, param.i_height ) < 0 )
        goto fail;

    h = x264_encoder_open( &param );
    if( !h )
        goto fail2;

    int luma_size = width * height;
    int chroma_size = luma_size / 4;

    /* Encode frames */
    while ( 1 ) {
        /* Read input frame */
        if( fread( pic.img.plane[0], 1, luma_size, stdin ) != (unsigned)luma_size )
            break;
        if( fread( pic.img.plane[1], 1, chroma_size, stdin ) != (unsigned)chroma_size )
            break;
        if( fread( pic.img.plane[2], 1, chroma_size, stdin ) != (unsigned)chroma_size )
            break;

        pic.i_pts = i_frame;
        i_frame_size = x264_encoder_encode( h, &nal, &i_nal, &pic, &pic_out );
        if( i_frame_size < 0 )
            goto fail3;
        else if( i_frame_size ) {
            if( !fwrite( nal->p_payload, i_frame_size, 1, stdout ) )
                goto fail3;
        }
        i_frame++;
    }

    /* Flush delayed frames */
    while( x264_encoder_delayed_frames( h ) ) {
        i_frame_size = x264_encoder_encode( h, &nal, &i_nal, NULL, &pic_out );
        if( i_frame_size < 0 )
            goto fail3;
        else if( i_frame_size ) {
            if( !fwrite( nal->p_payload, i_frame_size, 1, stdout ) )
                goto fail3;
        }
    }

    x264_encoder_close( h );
    x264_picture_clean( &pic );
    return 0;

fail3:
    x264_encoder_close( h );
fail2:
    x264_picture_clean( &pic );
fail:
    return -1;





//    int ret;
//    int y_size;
//    int i, j;
//
//    FILE* fp_src  = fopen("./sintel_640_360.yuv", "rb");
//    FILE* fp_dst = fopen("cuc_ieschool.h264", "wb");
//
//    //Encode 50 frame
//    //if set 0, encode all frame
//    int frame_num = 50;
//    int csp = X264_CSP_I420;
//    int width = 640, height = 360;
//
//    int iNal = 0;
//    x264_nal_t *pNals = NULL;
//    x264_t *pHandle = NULL;
//    x264_picture_t *pPic_in = (x264_picture_t*) malloc(sizeof(x264_picture_t));
//    x264_picture_t *pPic_out = (x264_picture_t*) malloc(sizeof(x264_picture_t));
//    x264_param_t *pParam = (x264_param_t*) malloc(sizeof(x264_param_t));
//
//    //Check
//    if(fp_src == NULL || fp_dst == NULL) {
//        printf("Error open files.\n");
//        return -1;
//    }
//
//    x264_param_default(pParam);
//    pParam->i_width = width;
//    pParam->i_height = height;
//    /*//Param
//    pParam->i_log_level = X264_LOG_DEBUG;
//    pParam->i_threads = X264_SYNC_LOOKAHEAD_AUTO;
//    pParam->i_frame_total = 0;
//    pParam->i_keyint_max = 10;
//    pParam->i_bframe = 5;
//    pParam->b_open_gop = 0;
//    pParam->i_bframe_pyramid = 0;
//    pParam->rc.i_qp_constant = 0;
//    pParam->rc.i_qp_max = 0;
//    pParam->rc.i_qp_min = 0;
//    pParam->i_bframe_adaptive = X264_B_ADAPT_TRELLIS;
//    pParam->i_fps_den = 1;
//    pParam->i_fps_num = 25;
//    pParam->i_timebase_den = pParam->i_fps_num;
//    pParam->i_timebase_num = pParam->i_fps_den;
//    */
//    pParam->i_csp = csp;
//    x264_param_apply_profile(pParam, x264_profile_names[5]);
//
//
//    pHandle = x264_encoder_open(pParam);
//
//    x264_picture_init(pPic_out);
//    x264_picture_alloc(pPic_in, csp, pParam->i_width, pParam->i_height);
//
//    //ret = x264_encoder_headers(pHandle, &pNals, &iNal);
//
//    y_size = pParam->i_width * pParam->i_height;
//    //detect frame number
//    if (frame_num == 0) {
//        fseek(fp_src, 0, SEEK_END);
//        switch (csp) {
//            case X264_CSP_I444:
//                frame_num = ftell(fp_src) / (y_size * 3);   //计算这个文件里一共包含多少帧的数据
//                break;
//            case X264_CSP_I420:
//                frame_num = ftell(fp_src) / (y_size * 3 / 2);
//                break;
//            default:
//                printf("Colorspace Not Support.\n");
//                return -1;
//        }
//        fseek(fp_src, 0, SEEK_SET);
//    }
//
//    //Loop to Encode
//    for(i=0; i < frame_num; i++) {
//        switch(csp) {
//            case X264_CSP_I444: {
//                fread(pPic_in->img.plane[0], y_size, 1, fp_src);         //Y
//                fread(pPic_in->img.plane[1], y_size, 1, fp_src);         //U
//                fread(pPic_in->img.plane[2], y_size, 1, fp_src);         //V
//                break;
//            }
//            case X264_CSP_I420: {
//                fread(pPic_in->img.plane[0], y_size, 1, fp_src);         //Y
//                fread(pPic_in->img.plane[1], y_size/4, 1, fp_src);     //U
//                fread(pPic_in->img.plane[2], y_size/4, 1, fp_src);     //V
//                break;
//            }
//            default: {
//                printf("Colorspace Not Support.\n");
//                return -1;
//            }
//        }
//        pPic_in->i_pts = i;
//
//        ret = x264_encoder_encode(pHandle, &pNals, &iNal, pPic_in, pPic_out);
//        if (ret < 0) {
//            printf("Error.\n");
//            return -1;
//        }
//        printf("Succeed encode frame: %5d\n",i);
//
//        for (j = 0; j < iNal; ++j) {
//            fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
//        }
//    }
//
//    i = 0;
//    //flush encoder
//    while (1) {
//        ret = x264_encoder_encode(pHandle, &pNals, &iNal, NULL, pPic_out);
//        if (ret == 0) {
//            break;
//        }
//        printf("Flush 1 frame.\n");
//        for (j = 0; j < iNal; ++j) {
//            fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
//        }
//        i++;
//    }
//
//    x264_picture_clean(pPic_in);
//    x264_encoder_close(pHandle);
//    pHandle = NULL;
//
//    free(pPic_in);
//    free(pPic_out);
//    free(pParam);
//
//    fclose(fp_src);
//    fclose(fp_dst);
//    return 0;
}

