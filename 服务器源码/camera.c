/*========================================================================
	> File Name: camera.c
	> Author:  
	> Mail: 1216951938@qq.com 
	> Created Time: 2020年02月28日 星期五 19时55分26秒
==========================================================================*/

#include "camera.h"
#include "convert.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>



#define   REQBUFS_COUNT   4 //申请内核空间帧缓冲数量

static int flag = 0;
/*该结构体用于记录摄像头真实的配置信息*/
static struct Camera_Info indeed_info = {640, 480, 640*480, MJPEG};
 
/*该结构体用于存储映射到用户空间的缓冲信息*/
struct video_buf {
    void *start;
    unsigned int length;
};

struct video_buf bufs[REQBUFS_COUNT]; //定义用户空间缓冲数组


/*该结构体用于缓冲图片信息*/
static struct v4l2_requestbuffers reqbufs;

static int camfd = -1; //摄像头的文件描述符



/*摄像头开始采集信息函数*/
static int Camera_Start(void)
{
    int ret;
    enum v4l2_buf_type type = V4L2_CAP_VIDEO_CAPTURE; //定义捕获类型
    ret = ioctl(camfd, VIDIOC_STREAMON, &type);
    if(0 > ret)
    {
        printf("error: Camera_Start() -> VIDIOC_STREAMON\n");
        close(camfd);
        return -1;
    }
    printf("Camera start success\n");
    return 0;
}


/*摄像头出队函数*/
static int Camera_Debuf(void **buf, unsigned int *index)
{
    int ret = 0;

    /*使用select()IO多路复用机制,保证fd有照片才出队*/
    fd_set fds; //监视多路IO集合
    struct timeval timeout; //超时结构体信息
    struct v4l2_buffer vbuf; //用于获取驱动一帧信息的地址信息
    while(1)
    {
        FD_ZERO(&fds); //每次循环都要清空，否则不能检测到文件描述符的变化
        FD_SET(camfd, &fds); //添加文件描述符
        timeout.tv_sec = 2; //设置超时时间为2，两秒进行依次轮循
        timeout.tv_usec = 0; //设置事件精度为0，非阻塞
        ret = select(camfd+1, &fds, NULL, NULL, &timeout);
        if(0 > ret)
        {
            printf("error: Camera_Debuf() -> select -> ret > 0\n");
            if(errno == EINTR) //select检测的错误是中断产生的
            {
                continue; //重新检测
            }else {
                return -1;
            }
        }else if(ret == 0) { //等待超时，没有照片可读
            printf("error: Camera_Debuf() -> select -> ret == 0\n");
            return -1;
        }else {
            vbuf.type = V4L2_CAP_VIDEO_CAPTURE; //视频捕捉方式
            vbuf.memory = V4L2_MEMORY_MMAP; //内存访问模式
            
            /*出队操作，将捕获好的照片内存拉出已捕获照片队列*/
            ret = ioctl(camfd, VIDIOC_DQBUF, &vbuf); 
            if(0 > ret)
            {
                printf("error: Camera_Debuf() -> VIOIOC_DQBUF\n");
                close(camfd);
                return -1;
            }
            *buf = bufs[vbuf.index].start;
            indeed_info.pic_size = vbuf.bytesused; //将缓冲区用的内存大小写给照片的实时信息
            *index = vbuf.index; //缓冲编号
            return 0;
        }
    }
}


/*摄像头入队函数*/
static int Camera_Enbuf(unsigned int index)
{
    int ret;
    struct v4l2_buffer vbuf;
    vbuf.type = V4L2_CAP_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    vbuf.index = index; //告知上次出队的节点

    ret = ioctl(camfd, VIDIOC_QBUF, &vbuf);
    if(0 > ret)
    {
        printf("error: Camera_Enbuf() -> VIDIOC_QBUF");
        return -1;
    }
    return 0;
}


/*摄像头停止采集函数*/
static int Camera_Stop(void)
{
    int ret;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
    ret = ioctl(camfd, VIDIOC_STREAMOFF, &type);
    if(0 > ret)
    {
        printf("error: Camera_Stop -> VIDIOC_STRWAMOFF\n");
        return -1;
    }
    printf("camera stop capture\n");
    return 0;
}


/*摄像头退出函数*/
static int Camera_Exit(void)
{
    int i;
    int ret;
    struct v4l2_buffer vbuf;
    vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vbuf.memory = V4L2_MEMORY_MMAP;
    for(i = 0; i < reqbufs.count; i++)
    {
        ret = ioctl(camfd, VIDIOC_DQBUF, &vbuf); //将所有的内核空间出队
        if(0 > ret)
        {
            printf("error: Camera_Exit() -> VIDIOC_DQBUF\n");
            break;
        }
    }
    for(i = 0; i < reqbufs.count; i++)
    {
        munmap(bufs[i].start, bufs[i].length); //取消所有内核
    }
    printf("error: Camera_Exit() -> munmap()\n");
    close(camfd);
}


/*摄像头初始化函数*/
int Camera_Init(char *pathname)
{
    /*1、打开设备*/
    camfd = open(pathname, O_RDWR);
    if(0 > camfd)
    {
        printf("error: Camera_Init -> open\n");
        return -1;
    }

    /*2、查看设备功能*/
    struct v4l2_capability capability;
    int ret; 
    ret = ioctl(camfd, VIDIOC_QUERYCAP, &capability); //查看驱动能力
    if(ret < 0)
    {
        printf("error: Camera_Init -> VIDIOC_QUERYCAP\n");
        close(camfd);
        return -1;
    }

    if(!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) //查看是否有图片捕获能力
    {
        printf("error: Camera_Init -> V4L2_CAP_VIDEO_CAPTURE\n");
        close(camfd);
        return -1;
    }
    if(!(capability.capabilities & V4L2_CAP_STREAMING)) //查看是否有采集视频的能力
    {
        printf("error: Camera_Init -> V4L2_CAP_STREAMING\n");
        close(camfd);
        return -1;
    }

    /*判断摄像头支持的格式*/
    struct v4l2_fmtdesc fmtdesc;
    char capture_stat[50] = {0};
    char pic_buf[20] = {0};
    fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format:\n");
    while(ioctl(camfd, VIDIOC_ENUM_FMT, &fmtdesc) != -1)
    {
	sprintf(pic_buf, "\t%d.%s\n", fmtdesc.index+1, fmtdesc.description);
	printf("\t%d.%s\n", fmtdesc.index+1, fmtdesc.description);
	strcat(capture_stat, pic_buf);
	fmtdesc.index++;
    }
    

    /*3、设置图片格式*/
    struct v4l2_format format;
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //设置视频捕获方式
    if(strstr(capture_stat, "MJPEG") != NULL)
    {
    	format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //'M', 'J', 'P', /设置数据格式 v4l2_fourcc('G')
   	printf("pic_type choose jpeg\n");
	flag = 1;
    }
    else
    {
    	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //设置数据格式 v4l2_fourcc('Y', 'U', 'Y', 'V')
   	printf("pic_type choose yuyv\n");
	flag = 2;
    }
    format.fmt.pix.field = V4L2_FIELD_ANY; // 逐行扫描
    format.fmt.pix.width = indeed_info.width;
    format.fmt.pix.height = indeed_info.height;

    ret = ioctl(camfd, VIDIOC_S_FMT, &format); //设置图片格式
    if(0 > ret)
    {
        printf("error: Camera_Init -> VIDIOC_S_FMT\n");
        close(camfd);
        return -1;
    }
    ret = ioctl(camfd, VIDIOC_G_FMT, format); //得到设置后的图片格式，检查之前的格式内核是否支持
#if 0
    indeed_info.width = format.fmt.pix.width; //获取设置后真实的宽度
    indeed_info.height = format.fmt.pix.height; //获取设置后真实的高低
    if(format.fmt.pix.pixelformat == V4L2_PIX_FMT_MJPEG)
    {
        indeed_info.pic_format = MJPEG;
    }else {
        indeed_info.pic_format = NJPEG;
        printf("error: Camera_Info -> V4L2_PIX_FMT_MJPEG\n");
        close(camfd);
        return -1;
    }
#endif
    /*4、申请帧缓冲*/
    memset(&reqbufs, 0, sizeof(reqbufs));
    reqbufs.count = REQBUFS_COUNT; //获得的缓存个数
    reqbufs.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //设置捕获方式，参数不变
    reqbufs.memory = V4L2_MEMORY_MMAP; //内存区的使用方式，为映射

    ret = ioctl(camfd, VIDIOC_REQBUFS, &reqbufs); //获得缓存空间
    if(0 > ret)
    {
        printf("error: Camera_Init -> VIDIOC_REQBUFS\n");
        close(camfd);
        return -1;
    }


    /*5、内存映射*/
    struct v4l2_buffer vbuf; //存储驱动中一帧的信息
    int i;
    for(i = 0; i < reqbufs.count; i++)
    {
        memset(&vbuf, 0, sizeof(vbuf));
        vbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; //捕获的方式
        vbuf.memory = V4L2_MEMORY_MMAP; //映射方式
        vbuf.index = i;
        ret = ioctl(camfd, VIDIOC_QUERYBUF, &vbuf); //获得缓冲的地址
        if(0 > ret)
        {
            printf("error: Camera_Info -> VIDIOC_QUERYCAP\n");
            close(camfd);
            return -1;
        }
        bufs[i].length = vbuf.length;
        bufs[i].start = mmap(NULL, vbuf.length, PROT_READ | PROT_WRITE, MAP_SHARED, camfd, vbuf.m.offset);
        if(bufs[i].start == MAP_FAILED)
        {
            printf("error: Camera_Init -> MAP_FAILED\n");
            close(camfd);
            return -1;
        }
    
    /*6、帧缓冲入队*/
        vbuf.type = V4L2_CAP_VIDEO_CAPTURE; //入队的捕获方式
        vbuf.memory = V4L2_MEMORY_MMAP; //入队的捕获方式的内核去使用模式
        ret = ioctl(camfd, VIDIOC_QBUF, &vbuf);
        if(0 > ret)
        {
            printf("error: Camera_Init -> VIDIOC_QBUF\n");
            close(camfd);
            return -1;
        }
    }

    indeed_info.pic_size = bufs[0].length; //重写图片真实的大小信息
    
    /*7、开始采集*/
    ret = Camera_Start();
    if(0 > ret)
        return -1;

    printf("Camera init success\n");
    printf("\n\npicture width: %d\n", indeed_info.width);
    printf("picture height: %d\n", indeed_info.height);
    printf("picture size: %d\n", indeed_info.pic_size);
    printf("picture format : %d\n", indeed_info.pic_format);

}

/*获取一张照片信息函数*/
struct Camera_Pic *Camera_Getpic(struct Camera_Pic *pic)
{
    char *yuv;
    char *rgb;
    char *jpeg;
    unsigned int index = 0;
    int ret;
#if 1
    if(flag == 1)
    {
	    //flag = 0;
	    ret = Camera_Debuf((void **)&jpeg, &index);
	    if(0 > ret)
	    {
		    printf("error: Camera_Getpic() -> Camera_Enbuf()\n");
		    goto erro;
	    }

	    /*将出队的照片大小和地址复制到发送给客户端图片信息结构体*/
	    memcpy(pic->buf, jpeg, indeed_info.pic_size);
	    pic->size = indeed_info.pic_size;
	    /*重新入队为了队列的循环采集*/
	    ret = Camera_Enbuf(index);
	    if(0 > ret)
	    {
		    printf("error: Camera_GetpicPic() -> Camera_Enbuf()\n");

	    }
	    return pic;
    }
#endif


#if 1
    else if(flag == 2)
    {
//	    flag = 0;
	    ret = Camera_Debuf((void **)&yuv, &index);
	    if(0 > ret)
	    {
		    printf("error: Camera_Getpic() -> Camera_Enbuf()\n");
		    goto erro;
	    }

	    rgb = malloc(640 * 480 * 3);
	    convert_rgb_to_jpg_init();
	    convert_yuv_to_rgb(yuv, rgb, 640, 480, 24);
	    pic->size = convert_rgb_to_jpg_work(rgb, pic->buf, 640, 480, 24, 80);
	    printf("pic -> size : %d", pic->size);
	    convert_rgb_to_jpg_exit();

	    /*重新入队为了队列的循环采集*/
	    ret = Camera_Enbuf(index);
	    if(0 > ret)
	    {
		    printf("error: Camera_GetpicPic() -> Camera_Enbuf()\n");

	    }
	    return pic;
    }
#endif
    
erro:
    Camera_Stop();
    Camera_Exit();
    return NULL;

}

