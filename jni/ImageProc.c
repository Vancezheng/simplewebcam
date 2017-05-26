#include "ImageProc.h"

int errnoexit(const char *s)
{
	LOGE("%s error %d, %s", s, errno, strerror (errno));
	return ERROR_LOCAL;
}


int xioctl(int fd, int request, void *arg)
{
	int r;

	do r = ioctl (fd, request, arg);
	while (-1 == r && EINTR == errno);

	return r;
}

int checkCamerabase(void){
	struct stat st;
	int i;
	int start_from_4 = 1;
	
	/* if /dev/video[0-3] exist, camerabase=4, otherwise, camrerabase = 0 */
	for(i=0 ; i<4 ; i++){
		sprintf(dev_name,"/dev/video%d",i);
		if (-1 == stat (dev_name, &st)) {
			start_from_4 &= 0;
		}else{
			start_from_4 &= 1;
		}
	}

	if(start_from_4){
		return 4;
	}else{
		return 0;
	}
}

int opendevice(int i)
{
	struct stat st;

	sprintf(dev_name,"/dev/video%d",i);

	if (-1 == stat (dev_name, &st)) {
		LOGE("Cannot identify '%s': %d, %s", dev_name, errno, strerror (errno));
		return ERROR_LOCAL;
	}

	if (!S_ISCHR (st.st_mode)) {
		LOGE("%s is no device", dev_name);
		return ERROR_LOCAL;
	}

	fd = open (dev_name, O_RDWR | O_NONBLOCK, 0);

	if (-1 == fd) {
		LOGE("Cannot open '%s': %d, %s", dev_name, errno, strerror (errno));
		return ERROR_LOCAL;
	}
	return SUCCESS_LOCAL;
}

static int float_to_fraction_recursive(double f, double p, int *num, int *den)
{
	int whole = (int)f;
	f = fabs(f - whole);

	if(f > p) {
		int n, d;
		int a = float_to_fraction_recursive(1 / f, p + p / f, &n, &d);
		*num = d;
		*den = d * a + n;
	}
	else {
		*num = 0;
		*den = 1;
	}
	return whole;
}

void float_to_fraction(float f, int *num, int *den)
{
	int whole = float_to_fraction_recursive(f, FLT_EPSILON, num, den);
	*num += whole * *den;
}

int initdevice(void) 
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	struct v4l2_streamparm params;
    int fps;
    unsigned int rate_denominator=10;
    unsigned int rate_numerator=1;
	unsigned int min;

	if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			LOGE("%s is no V4L2 device", dev_name);
			return ERROR_LOCAL;
		} else {
			return errnoexit ("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		LOGE("%s is no video capture device", dev_name);
		return ERROR_LOCAL;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		LOGE("%s does not support streaming i/o", dev_name);
		return ERROR_LOCAL;
	}
	
	CLEAR (cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; 

		if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
				case EINVAL:
					break;
				default:
					break;
			}
		}
	} else {
	}

	CLEAR (fmt);

	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	fmt.fmt.pix.width       = img_width;
	fmt.fmt.pix.height      = img_height;

	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;//V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	if (-1 == xioctl (fd, VIDIOC_S_FMT, &fmt))
		return errnoexit ("VIDIOC_S_FMT");

#if 1
    /* set frame rate. */
    CLEAR(params);
    params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    float_to_fraction(30, (int *)&rate_denominator, (int *)&rate_numerator);
    params.parm.capture.timeperframe.numerator = rate_numerator;
    params.parm.capture.timeperframe.denominator = rate_denominator;
	if(xioctl(fd, VIDIOC_S_PARM, &params) == -1)
	{
		LOGE("Failed to set video parameters (VIDIOC_S_PARM)");
	}

	if (xioctl(fd, VIDIOC_G_PARM, &params) != -1) {
	    LOGI("VIDIOC_G_PARM : %d %d %d\n", params.type, params.parm.capture.timeperframe.numerator, params.parm.capture.timeperframe.denominator);
    }
#else
	/* set framerate */
	struct v4l2_streamparm* setfps;
	setfps=(struct v4l2_streamparm *) calloc(1, sizeof(struct v4l2_streamparm));
	memset(setfps, 0, sizeof(struct v4l2_streamparm));
	setfps->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	// Convert the frame rate into a fraction for V4L2
	int n = 0, d = 0;
    fps = 30;
	float_to_fraction(fps, &n, &d);
	setfps->parm.capture.timeperframe.numerator = d;
	setfps->parm.capture.timeperframe.denominator = n;

    int ret;
	ret = ioctl(fd, VIDIOC_S_PARM, setfps);
	if(ret == -1) {
		LOGE("Unable to set frame rate");
	}
	ret = ioctl(fd, VIDIOC_G_PARM, setfps);
	if(ret == 0) {
	    LOGI("VIDIOC_G_PARM : %d %d %d\n", params.type, params.parm.capture.timeperframe.numerator, params.parm.capture.timeperframe.denominator);
		float confirmed_fps = (float)setfps->parm.capture.timeperframe.denominator / (float)setfps->parm.capture.timeperframe.numerator;
		if (confirmed_fps != (float)n / (float)d) {
			printf("  Frame rate:   %g fps (requested frame rate %g fps is "
				"not supported by device)\n",
				confirmed_fps,
				fps);
			fps = confirmed_fps;
		}
		else {
			LOGI("Frame rate: %d fps\n", fps);
		}
	}
	else {
		LOGE("Unable to read out current frame rate");
	}
#endif

	return initmmap ();

}

int initmmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR (req);

	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			LOGE("%s does not support memory mapping", dev_name);
			return ERROR_LOCAL;
		} else {
			return errnoexit ("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		LOGE("Insufficient buffer memory on %s", dev_name);
		return ERROR_LOCAL;
 	}

	buffers = (struct buffer *)calloc (req.count, sizeof (*buffers));

	if (!buffers) {
		LOGE("Out of memory");
		return ERROR_LOCAL;
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		 CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl (fd, VIDIOC_QUERYBUF, &buf))
			return errnoexit ("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
		mmap (NULL ,
			buf.length,
			PROT_READ | PROT_WRITE,
			MAP_SHARED,
			fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			return errnoexit ("mmap");
        
        //memset(buffers[n_buffers].start, 0xab, buf.length);
	}

	return SUCCESS_LOCAL;
}

int startcapturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;

		CLEAR (buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = i;

		if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
			return errnoexit ("VIDIOC_QBUF");
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (fd, VIDIOC_STREAMON, &type))
		return errnoexit ("VIDIOC_STREAMON");

	return SUCCESS_LOCAL;
}

int readframeonce(void)
{
	for (;;) {
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO (&fds);
		FD_SET (fd, &fds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		r = select (fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r) {
			if (EINTR == errno)
				continue;

			return errnoexit ("select");
		}

		if (0 == r) {
			LOGE("select timeout");
			//return ERROR_LOCAL;
            continue;
		}

		if (readframe()==1) {
			break;
		}
	}

	return SUCCESS_LOCAL;

}


void processimage (const void *p, int size)
{
		//yuyv422toABGRY((unsigned char *)p);
    //mjpegtoABGRY((unsigned char *)p);
    //LOGI("process image size=%d", size);
    framedata = (unsigned char*) malloc(size);
    memcpy(framedata, (unsigned char *)p, size);
    framesize = size;
}

int readframe(void)
{

	struct v4l2_buffer buf;
	unsigned int i;

	CLEAR (buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl (fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				return errnoexit ("VIDIOC_DQBUF");
		}
	}

	assert (buf.index < n_buffers);

	processimage (buffers[buf.index].start, buf.bytesused);

	if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
		return errnoexit ("VIDIOC_QBUF");

	return 1;
}

int stopcapturing(void)
{
	enum v4l2_buf_type type;

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
		return errnoexit ("VIDIOC_STREAMOFF");

	return SUCCESS_LOCAL;

}

int uninitdevice(void)
{
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap (buffers[i].start, buffers[i].length))
			return errnoexit ("munmap");

	free (buffers);

	return SUCCESS_LOCAL;
}

int closedevice(void)
{
	if (-1 == close (fd)){
		fd = -1;
		return errnoexit ("close");
	}

	fd = -1;
	return SUCCESS_LOCAL;
}



void yuyv422toABGRY(unsigned char *src)
{

	int width=0;
	int height=0;

	width = IMG_WIDTH;
	height = IMG_HEIGHT;

	int frameSize =width*height*2;

	int i;

	if((!rgb || !ybuf)){
		return;
	}
	int *lrgb = NULL;
	int *lybuf = NULL;
		
	lrgb = &rgb[0];
	lybuf = &ybuf[0];

	if(yuv_tbl_ready==0){
		for(i=0 ; i<256 ; i++){
			y1192_tbl[i] = 1192*(i-16);
			if(y1192_tbl[i]<0){
				y1192_tbl[i]=0;
			}

			v1634_tbl[i] = 1634*(i-128);
			v833_tbl[i] = 833*(i-128);
			u400_tbl[i] = 400*(i-128);
			u2066_tbl[i] = 2066*(i-128);
		}
		yuv_tbl_ready=1;
	}

	for(i=0 ; i<frameSize ; i+=4){
		unsigned char y1, y2, u, v;
		y1 = src[i];
		u = src[i+1];
		y2 = src[i+2];
		v = src[i+3];

		int y1192_1=y1192_tbl[y1];
		int r1 = (y1192_1 + v1634_tbl[v])>>10;
		int g1 = (y1192_1 - v833_tbl[v] - u400_tbl[u])>>10;
		int b1 = (y1192_1 + u2066_tbl[u])>>10;

		int y1192_2=y1192_tbl[y2];
		int r2 = (y1192_2 + v1634_tbl[v])>>10;
		int g2 = (y1192_2 - v833_tbl[v] - u400_tbl[u])>>10;
		int b2 = (y1192_2 + u2066_tbl[u])>>10;

		r1 = r1>255 ? 255 : r1<0 ? 0 : r1;
		g1 = g1>255 ? 255 : g1<0 ? 0 : g1;
		b1 = b1>255 ? 255 : b1<0 ? 0 : b1;
		r2 = r2>255 ? 255 : r2<0 ? 0 : r2;
		g2 = g2>255 ? 255 : g2<0 ? 0 : g2;
		b2 = b2>255 ? 255 : b2<0 ? 0 : b2;

		*lrgb++ = 0xff000000 | b1<<16 | g1<<8 | r1;
		*lrgb++ = 0xff000000 | b2<<16 | g2<<8 | r2;

		if(lybuf!=NULL){
			*lybuf++ = y1;
			*lybuf++ = y2;
		}
	}

}

/* ISO/IEC 10918-1:1993(E) K.3.3. Default Huffman tables used by MJPEG UVC devices
 which don't specify a Huffman table in the JPEG stream. */
static const unsigned char dc_lumi_len[] = {
	0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const unsigned char dc_lumi_val[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const unsigned char dc_chromi_len[] = {
	0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const unsigned char dc_chromi_val[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

static const unsigned char ac_lumi_len[] = {
	0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
static const unsigned char ac_lumi_val[] = {
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11,	0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71,	0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};
static const unsigned char ac_chromi_len[] = {
	0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
static const unsigned char ac_chromi_val[] = {
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

#define COPY_HUFF_TABLE(dinfo,tbl,name) do { \
	if (dinfo->tbl == NULL) dinfo->tbl = jpeg_alloc_huff_table((j_common_ptr)dinfo); \
		memcpy(dinfo->tbl->bits, name##_len, sizeof(name##_len)); \
		memset(dinfo->tbl->huffval, 0, sizeof(dinfo->tbl->huffval)); \
		memcpy(dinfo->tbl->huffval, name##_val, sizeof(name##_val)); \
	} while(0)

static inline void insert_huff_tables(j_decompress_ptr dinfo) {
	COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[0], dc_lumi);
	COPY_HUFF_TABLE(dinfo, dc_huff_tbl_ptrs[1], dc_chromi);
	COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[0], ac_lumi);
	COPY_HUFF_TABLE(dinfo, ac_huff_tbl_ptrs[1], ac_chromi);
}

void mjpegtoABGRY(unsigned char *src) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    unsigned char *buffer;
    unsigned int x;
    int *lrgb = NULL;

    lrgb = &rgb[0];

    cinfo.err = jpeg_std_error(&jerr);

	jpeg_create_decompress(&cinfo);
	jpeg_mem_src(&cinfo, src, img_width * img_height * 3);
	jpeg_read_header(&cinfo, TRUE);

	if (cinfo.dc_huff_tbl_ptrs[0] == NULL) {
		/* This frame is missing the Huffman tables: fill in the standard ones */
        LOGI("insert huffman tables");
		insert_huff_tables(&cinfo);
	}

	//cinfo.out_color_space = JCS_RGB;
	cinfo.out_color_space = JCS_RGBA_8888;
	cinfo.dct_method = JDCT_IFAST;
    LOGI("cinfo.jpeg_color_space=%d cinfo.out_color_space=%d cinfo.num_components=%d", \
            cinfo.jpeg_color_space, cinfo.out_color_space, cinfo.num_components);
	jpeg_start_decompress(&cinfo);

    buffer = (unsigned char *) malloc(cinfo.output_width * cinfo.output_components);
    while (cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines(&cinfo, &buffer, 1);
        for (x = 0; x < cinfo.output_width; x++) {
            //*lrgb++ = 0xff000000 | buffer[x] | buffer[x * 3 + 1] << 8 | buffer[x * 3] << 16;
            *lrgb++ = buffer[x * 4] | buffer[x * 4 + 1] << 8 | buffer[x * 4 + 2] << 16 | buffer[x * 4 + 3] << 24;
            //*lrgb++ = 0xff000000 | buffer[x] << 16 | buffer[x * 3 + 1] << 8 | buffer[x * 3];
            //LOGI("*lrgb=0x%x", *(lrgb-1));
        }
    }

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
    free(buffer);
}

void 
Java_com_camera_simplewebcam_CameraPreview_pixeltobmp( JNIEnv* env,jobject thiz,jobject bitmap){

	jboolean bo;


	AndroidBitmapInfo  info;
	void*              pixels;
	int                ret;
	int i;
	int *colors;

	int width=0;
	int height=0;

	if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
		LOGE("AndroidBitmap_getInfo() failed ! error=%d", ret);
		return;
	}
    
	width = info.width;
	height = info.height;

	if(!rgb || !ybuf) return;

	if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
		LOGE("Bitmap format is not RGBA_8888 !");
		return;
	}

	if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixels)) < 0) {
		LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
	}

	colors = (int*)pixels;
	int *lrgb =NULL;
	lrgb = &rgb[0];

	for(i=0 ; i<width*height ; i++){
		*colors++ = *lrgb++;
	}

	AndroidBitmap_unlockPixels(env, bitmap);
}

jint 
Java_com_camera_simplewebcam_CameraPreview_powerOnOffCamera( JNIEnv* env,jobject thiz, jint power){
    
    int fd;
    int ret = -1;

    LOGI("power camera %d", power);
    fd = open(CAMERA_POWER_PATH, O_WRONLY);
	if (-1 == fd) {
		LOGE("Cannot open '%s': %d, %s", CAMERA_POWER_PATH, errno, strerror (errno));
		return ret;
	}
    
    if(power == 1) {
        ret = write(fd, "1", 1);
    } else if (power == 0){
        ret = write(fd, "0", 1);
    }

	if (1 != ret) {
		LOGE("Cannot open '%s': %d, %s", CAMERA_POWER_PATH, errno, strerror (errno));
        ret = 0;
	}
    close(fd);
    
    return ret;
}

jint 
Java_com_camera_simplewebcam_CameraPreview_prepareCamera( JNIEnv* env,jobject thiz, jint videoid){

	int ret;

	if(camerabase<0){
		camerabase = checkCamerabase();
	}

    while(1) {
        ret = opendevice(camerabase + videoid);
        if (ret == SUCCESS_LOCAL) {
            break;
        }
        usleep(100000);
        LOGI("sleep 100ms");
    }

	if(ret != ERROR_LOCAL){
		ret = initdevice();
	}
	if(ret != ERROR_LOCAL){
		ret = startcapturing();

		if(ret != SUCCESS_LOCAL){
			stopcapturing();
			uninitdevice ();
			closedevice ();
			LOGE("device resetted");	
		}

	}

	//if(ret != ERROR_LOCAL){
	//	rgb = (int *)malloc(sizeof(int) * (IMG_WIDTH*IMG_HEIGHT));
	//	ybuf = (int *)malloc(sizeof(int) * (IMG_WIDTH*IMG_HEIGHT));
	//}
	return ret;
}	



jint 
Java_com_camera_simplewebcam_CameraPreview_prepareCameraWithBase( JNIEnv* env,jobject thiz, jint videoid, jint videobase, jint width, jint height){
	
		int ret;

		camerabase = videobase;
	    img_width = width;
	    img_height = height;
		return Java_com_camera_simplewebcam_CameraPreview_prepareCamera(env,thiz,videoid);
	
}

jbyteArray
Java_com_camera_simplewebcam_CameraPreview_processCamera( JNIEnv* env,
										jobject thiz){
	readframeonce();
	jbyteArray array = (*env)->NewByteArray(env, framesize);
	if(array == NULL) {
        LOGE("Couldn't allocate byte array");
	}
	(*env)->SetByteArrayRegion(env, array, 0, framesize, framedata);
	if(framedata) free(framedata);

	return array;
}

void 
Java_com_camera_simplewebcam_CameraPreview_stopCamera(JNIEnv* env,jobject thiz){

	stopcapturing ();

	uninitdevice ();

	closedevice ();

	//if(rgb) free(rgb);
	//if(ybuf) free(ybuf);

	fd = -1;

}

