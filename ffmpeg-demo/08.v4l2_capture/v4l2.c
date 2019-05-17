/*
 *  V4L2 video capture example
 *  get yuyv images from v4l2 use linux v4l2 spec(not through ffmpeg)
 *    and encode to h264 video file use ffmpeg.
 *
 *  This program can be used and distributed without restrictions.
 */
//https://blog.csdn.net/ddddwant/article/details/8475211
//https://www.cnblogs.com/emouse/archive/2013/03/04/2943243.html
//fang: use "luvcview -L" to query which format your camera support.
//      "luvcview" will call a sdl window to show camera.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h> //getopt_long()
#include <fcntl.h>	//low-level i/o
#include <unistd.h>	//getpagesize()
#include <signal.h> //signal()
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          //for videodev2.h
#include <linux/videodev2.h>	//V4L2 的相关定义

#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

#define CLEAR(x) memset (&(x), 0, sizeof (x))

typedef enum {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;

struct mybuffer {
	void * start;
	size_t length;
};

static char * dev_name = NULL;
static io_method io = IO_METHOD_MMAP;	//io 默认是 mmap
static int fd = -1;
//IO_METHOD_MMAP/IO_METHOD_USERPTR	模式下: buffers 是 buffer[]数组,n_buffers 是数组 item 个数
//IO_METHOD_READ 					模式下: buffers 指向1个 buffer 结构体
struct mybuffer *buffers = NULL;
static unsigned int n_buffers = 0;

//ffmpeg
static AVFrame *frame = NULL;
static AVPacket *pkt = NULL;
AVCodecContext *c = NULL;
FILE *f;
char bInterrupted = 0;

static void CtrlCCatcher(int signal)
{
	bInterrupted = 1;
}

static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, FILE *outfile)
{
    int ret;
    /* send the frame to the encoder */
    if (frame)
        printf("Send frame %3"PRId64"\n", frame->pts);
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        fprintf(stderr, "Error sending a frame for encoding\n");
        exit(1);
    }
    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            fprintf(stderr, "Error during encoding\n");
            exit(1);
        }
        printf("Write packet %3"PRId64" (size=%5d)\n", pkt->pts, pkt->size);
        fwrite(pkt->data, 1, pkt->size, outfile);
        av_packet_unref(pkt);
    }
}

static void errno_exit(const char *s)
{
	fprintf (stderr, "%s error %d, %s\n", s, errno, strerror (errno));
	exit (EXIT_FAILURE);
}

static int xioctl (int fd, int request, void *arg)
{
	int r;	//response
	do{
		r = ioctl (fd, request, arg);	
	} while (-1 == r && EINTR == errno);
	return r;
}

static int process_image(void *addr, int length)
{
    int ret, x, y;
	static int num=0;
	printf("%d,get %d bytes data.\n",num++,length);
    /* make sure the frame data is writable */
    ret = av_frame_make_writable(frame);
    if (ret < 0)
        exit(1);
    /* Y */
    for (y = 0; y < c->height; y++) {
        for (x = 0; x < c->width; x++) {
            frame->data[0][y * frame->linesize[0] + x] = *(unsigned char *)((unsigned short *)addr + y*640+x);
        }
    }
    //Cb and Cr 
    for (y = 0; y < c->height/2; y++) {
        for (x = 0; x < c->width/2; x++) {
            //frame->data[1][y * frame->linesize[1] + x] = 128;
            //frame->data[2][y * frame->linesize[2] + x] = 128;
            frame->data[1][y * frame->linesize[1] + x] = *((unsigned char *)((unsigned short *)addr + y*640+x)+1);
            frame->data[2][y * frame->linesize[2] + x] = *((unsigned char *)((unsigned short *)addr + y*640+x)+1);
        }
    }
    frame->pts = num;
    /* encode the image */
    encode(c, frame, pkt, f);
	return 0;
}

static int read_frame(void)
{
	struct v4l2_buffer buf;
	unsigned int i;

	switch (io) {
		case IO_METHOD_READ:
			if (-1 == read (fd, buffers[0].start, buffers[0].length)) {
				switch (errno) {
					case EAGAIN:
						return 0;
					case EIO:  //Could ignore EIO, see spec. fall through
					default:
						errno_exit ("read");
				}
			}
			process_image (buffers[0].start, buffers[0].length);
			break;

		case IO_METHOD_MMAP:
			CLEAR (buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;

			if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {	//VIDIOC_DQBUF 从队列中取出帧
				switch (errno) {
					case EAGAIN:
						return 0;
					case EIO:	//Could ignore EIO, see spec. fall through
					default:
						errno_exit ("VIDIOC_DQBUF");
				}
			}
			assert (buf.index < n_buffers);
			process_image (buffers[buf.index].start, buffers[buf.index].length);

			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))		//VIDIOC_QBUF 把帧放入队列
				errno_exit ("VIDIOC_QBUF");
			break;

		case IO_METHOD_USERPTR:
			CLEAR (buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;

			if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {	//VIDIOC_DQBUF 从队列中取出帧
				switch (errno) {
					case EAGAIN:
						return 0;
					case EIO:	//Could ignore EIO, see spec. fall through
					default:
						errno_exit ("VIDIOC_DQBUF");
				}
			}

			for (i = 0; i < n_buffers; ++i){
				if (buf.m.userptr == (unsigned long) buffers[i].start && buf.length == buffers[i].length)
					break;
			}
			assert (i < n_buffers);
			//process_image ((void *) buf.m.userptr);
			process_image (buffers[i].start, buffers[i].length);

			if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))		//VIDIOC_QBUF 把帧放入队列
				errno_exit ("VIDIOC_QBUF");
			break;
	}
	return 1;
}

static void mainloop(void)
{
    signal(SIGINT, CtrlCCatcher);

	//init ffmpeg
    const char *filename="output.h264", *codec_name="libx264";
    const AVCodec *codec;
    int ret;
    /* find the mpeg1video encoder */
    codec = avcodec_find_encoder_by_name(codec_name);
    if (!codec) {
        fprintf(stderr, "Codec '%s' not found\n", codec_name);
        exit(1);
    }
    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
    pkt = av_packet_alloc();
    if (!pkt)
        exit(1);
    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 640;
    c->height = 480;
    /* frames per second */
    c->time_base = (AVRational){1, 25};//For fixed-fps content,timebase should be 1/framerate and timestamp increments should be identically 1.
    c->framerate = (AVRational){25, 1};
    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
    c->gop_size = 10;
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;//;
    if (codec->id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);
    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec: %s\n", av_err2str(ret));
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width  = c->width;
    frame->height = c->height;
    ret = av_frame_get_buffer(frame, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate the video frame data\n");
        exit(1);
    }

	while (bInterrupted == 0) {
		for (;;) {
			fd_set fds;
			struct timeval tv;
			int r;

			FD_ZERO (&fds);
			FD_SET (fd, &fds);

			/* Timeout. */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select (fd + 1, &fds, NULL, NULL, &tv);

			if (-1 == r) {
				if (EINTR == errno)
					continue;
				errno_exit ("select");
			}

			if (0 == r) {
				fprintf (stderr, "select timeout\n");
				exit (EXIT_FAILURE);
			}

			if (read_frame ())
				break;
			/* EAGAIN - continue select loop. */
		}
	}
    /* flush the encoder */
    encode(c, NULL, pkt, f);
    //fang: add sequence end code to have a real MPEG file(if your video file is xx.mp4)
    //uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    //fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);
    avcodec_free_context(&c);
    av_frame_free(&frame);
    av_packet_free(&pkt);
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;
	switch (io) {
		case IO_METHOD_READ:
			// Nothing to do.
			break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
				errno_exit ("VIDIOC_STREAMOFF");
			break;
	}
}

static void start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {
		case IO_METHOD_READ:
			// Nothing to do. 这个模式下直接调用 read 相关的 api 就可以了吧
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR (buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;

				if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
					errno_exit ("VIDIOC_QBUF");
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			//stream on, 开始捕获视频,然后摄像头就会不断向几块内核buffer中循环写入数据(已被映射到buffers[])
			if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
				errno_exit ("VIDIOC_STREAMON");
			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i) {
				struct v4l2_buffer buf;

				CLEAR (buf);

				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_USERPTR;
				buf.index = i;
				buf.m.userptr = (unsigned long) buffers[i].start;
				buf.length = buffers[i].length;

				if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
					errno_exit ("VIDIOC_QBUF");
			}
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			//stream on, 开始捕获视频,然后摄像头就会不断向几块buffer中循环写入数据
			if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
				errno_exit ("VIDIOC_STREAMON");
			break;
	}
}

static void uninit_device(void)
{
	unsigned int i;
	switch (io) {
		case IO_METHOD_READ:
			free (buffers[0].start);
			break;

		case IO_METHOD_MMAP:
			for (i = 0; i < n_buffers; ++i)
				if (-1 == munmap (buffers[i].start, buffers[i].length))
					errno_exit ("munmap");
			break;

		case IO_METHOD_USERPTR:
			for (i = 0; i < n_buffers; ++i)
				free (buffers[i].start);
			break;
	}
	free (buffers);	//数组本身也是malloc的
}

//为 全局 buffer(buffers) 分配结构体并 malloc buffer_size 字节的内存
static void init_read(unsigned int buffer_size)
{
	buffers = calloc (1, sizeof (*buffers));
	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc (buffer_size);

	if (!buffers[0].start) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;
	CLEAR (req);

	req.count = 4;					//请求 4 块 buffer
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;	//请求的 buffer 用于 video capture
	req.memory = V4L2_MEMORY_MMAP;	//这块 buffer 位于内核空间,需要将其 mmap 到用户空间

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {	//需要查询 本设备是否支持 mmap 的使用方法
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support "
				"memory mapping\n", dev_name);
				exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}
	if (req.count < 2) {	//没有那么多块 buffer 可供使用
		fprintf (stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit (EXIT_FAILURE);
	}

	//成功了,那么全局 buffers 就是一个 buffer 数组
	buffers = calloc (req.count, sizeof (*buffers));
	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR (buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			errno_exit ("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		//将设备内存映射到用户空间 buffers[] 上来
		buffers[n_buffers].start = mmap (	NULL /* start anywhere */,
											buf.length,
											PROT_READ | PROT_WRITE /* required */,
											MAP_SHARED /* recommended */,
											fd, buf.m.offset);	//设备驱动程序需要实现 mmap 方法
		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit ("mmap");
	}
}

static void init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);	//buffer_size 向上与 page_size 做对齐

	CLEAR (req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {	//需要查询 本设备是否支持 userptr 的使用方法
		if (EINVAL == errno) {
			fprintf (stderr, "%s does not support user pointer i/o\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_REQBUFS");
		}
	}

	buffers = calloc (4, sizeof (*buffers));
	if (!buffers) {
		fprintf (stderr, "Out of memory\n");
		exit (EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign (/* boundary */ page_size, buffer_size);	//分配内存并且其实地址 会对齐到 page_size

		if (!buffers[n_buffers].start) {
			fprintf (stderr, "Out of memory\n");
			exit (EXIT_FAILURE);
		}
	}
}

//初始化已经打开的全局字符设备 fd, 包括:
//1.查询设备能力,并设置相应的参数
//2.根据全局 io (默认是 IO_METHOD_MMAP) 的设置 init_read() 或 init_mmap() 或 init_userp()
static void init_device(void)
{
	struct v4l2_capability cap;		//查询设备属性
	struct v4l2_cropcap cropcap;	//用来查询摄像头的捕捉能力
	struct v4l2_crop crop;			//用来设置摄像头的捕捉能力
	struct v4l2_format fmt;			//用来设置摄像头的视频制式、帧格式等
	unsigned int min;

	//查询设备属性 query capability
	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {	//fang: xioctl 只是包装了一下 ioctl(),拿不到结果就一直 query
		if (EINVAL == errno) {
			fprintf (stderr, "%s is no V4L2 device\n", dev_name);
			exit (EXIT_FAILURE);
		} else {
			errno_exit ("VIDIOC_QUERYCAP");
		}
	}
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {	//这个 v4l2 要有 VIDEO_CAPTURE 图像获取能力
		fprintf (stderr, "%s is no video capture device\n", dev_name);
		exit (EXIT_FAILURE);
	}

	//继续查询(结合全局io的设置)
	switch (io) {	//
		case IO_METHOD_READ:
			if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
				fprintf (stderr, "%s does not support read i/o\n", dev_name);
				exit (EXIT_FAILURE);
			}
			break;
		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
			if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
				fprintf (stderr, "%s does not support streaming i/o\n", dev_name);
				exit (EXIT_FAILURE);
			}
			break;
	}
	fprintf(stdout,"camera info:\n");
	fprintf(stdout,"device driver=%s\n",cap.driver);
	fprintf(stdout,"device name=%s\n",cap.card);
	fprintf(stdout,"bus info=%s\n",cap.bus_info);

	CLEAR (cropcap);
	//(1/3).要查询设备捕捉视频的能力,先设置 type 为视频捕捉
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	//(2/3).获取设备捕捉能力的参数存于 v4l2_cropcap, 其中包括了
	//  bounds(最大捕捉框的左上角坐标和宽高),defrect(默认捕捉方框的左上角坐标和宽高)等信息
	if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {

		//(3/3).查询完成后就是 set crop
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;	// reset to default, defrect = default rectangle(稍微小于最大取样范围)
		if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {	//set crop
			switch (errno) {
				case EINVAL:
					//Cropping not supported.
					break;
				default:
					//Errors ignored.
					break;
			}
		}
	} else {
		//Errors ignored.
	}

	CLEAR (fmt);
	//v4l2_format 结构体用来设置摄像头的视频制式、帧格式等
	//在设置这个参数时应先填 好 v4l2_format 的各个域,如 type(传输流类型),fmt.pix.width(宽),
	//fmt.pix.heigth(高),fmt.pix.field(采样区域,如隔行采样),fmt.pix.pixelformat(采样类型，如 YUV4:2:2)
	//然后通过 VIDIO_S_FMT 操作命令设置视频捕捉格式
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 640; 
	fmt.fmt.pix.height = 480;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

	if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
		errno_exit ("VIDIOC_S_FMT");

	//Note VIDIOC_S_FMT may change width and height.
	//Buggy driver paranoia.
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	//set frame rate
	struct v4l2_streamparm streamparam;
	streamparam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	streamparam.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	streamparam.parm.capture.timeperframe.denominator = 15;
	streamparam.parm.capture.timeperframe.numerator = 1;
	streamparam.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
	if (-1 == xioctl (fd, VIDIOC_S_PARM, &streamparam))
		errno_exit ("VIDIOC_S_FMT");

	switch (io) {
		case IO_METHOD_READ:
			init_read (fmt.fmt.pix.sizeimage);	//初始化全局内存
			break;

		case IO_METHOD_MMAP:
			init_mmap ();
			break;

		case IO_METHOD_USERPTR:
			init_userp (fmt.fmt.pix.sizeimage);
			break;
	}
}

//将打开的字符设备全局 fd 关闭
static void close_device(void)
{
	if (-1 == close (fd))
		errno_exit ("close");
	fd = -1;
}

//打开字符设备 dev_name (dev_name 是全局 static 变量)
//设备打开后,用全局 fd 记录
static void open_device(void)
{
	struct stat st;
	if (-1 == stat (dev_name, &st)) {	//fang: 获取文件信息失败,失败原因在 errno
		fprintf (stderr, "Cannot identify '%s': %d, %s\n", dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
	if (!S_ISCHR (st.st_mode)) {	//应该是一个 字符设备?
		fprintf (stderr, "%s is no device\n", dev_name);
		exit (EXIT_FAILURE);
	}

	fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);	//打开这个字符设备
	if (-1 == fd) {
		fprintf (stderr, "Cannot open '%s': %d, %s\n", dev_name, errno, strerror (errno));
		exit (EXIT_FAILURE);
	}
}

static void usage(FILE *fp, int argc, char **argv)
{
	fprintf (fp,
		"Usage: %s [options]\n\n"
		"Options:\n"
		"-d | --device name   Video device name [/dev/video]\n"
		"-h | --help          Print this message\n"
		"-m | --mmap          Use memory mapped buffers\n"
		"-r | --read          Use read() calls\n"
		"-u | --userp         Use application allocated buffers\n"
		"", argv[0]);
}

static const char short_options [] = "d:hmru";

static const struct option long_options [] = {
	{ "device",     required_argument,      NULL,           'd' },
	{ "help",       no_argument,            NULL,           'h' },
	{ "mmap",       no_argument,            NULL,           'm' },
	{ "read",       no_argument,            NULL,           'r' },
	{ "userp",      no_argument,            NULL,           'u' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
	dev_name = "/dev/video";	//指定了一个默认的输入设备
	for (;;) {
		int index;
		int c;
		c = getopt_long (argc, argv, short_options, long_options, &index);
		if (-1 == c)	//-1 代表获取失败吧
			break;
		switch (c) {
			case 0: /* getopt_long() flag */
				break;
			case 'd':
				dev_name = optarg;
				break;
			case 'h':
				usage (stdout, argc, argv);
				exit (EXIT_SUCCESS);
			case 'm':
				io = IO_METHOD_MMAP;
				break;
			case 'r':
				io = IO_METHOD_READ;
				break;
			case 'u':
				io = IO_METHOD_USERPTR;
				break;
			default:
				usage (stderr, argc, argv);
				exit (EXIT_FAILURE);
		}
	}
	open_device ();			//
	init_device ();			//==
	start_capturing ();		//====
	mainloop ();
	stop_capturing ();		//====
	uninit_device ();		//==
	close_device ();		//
	exit (EXIT_SUCCESS);
	return 0;
}


