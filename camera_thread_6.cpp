#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <sys/wait.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <semaphore.h>
#include <signal.h>
#include <time.h>

#define IMAGEWIDTH 1920
#define IMAGEHEIGHT 1080

using namespace std;
using namespace cv;

char *camera_device[] = {
        "/dev/video0",
        "/dev/video1",
        "/dev/video2",
        "/dev/video3",
	    "/dev/video4",
    	"/dev/video5",
};

Mat imageShow(IMAGEHEIGHT,IMAGEWIDTH,CV_8UC3);

void *show_camera(void *arg)
{

    int fd;
    int i = *(int *)arg;
    struct v4l2_capability cap;
    struct v4l2_format fmt;
    struct v4l2_fmtdesc fmtdesc;
    enum   v4l2_buf_type type;

    if ((fd = open(camera_device[i], O_RDWR)) == -1){
        printf("Opening video device error\n");
        return NULL;
    }
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1){
        printf("unable Querying Capabilities\n");
        return NULL;
    }
    else
        fmtdesc.index = 0;
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    printf("Support format: \n");
    while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc) != -1){
        printf("\t%d. %s\n",fmtdesc.index+1,fmtdesc.description);
        fmtdesc.index++;
    }

    //set fmt
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = IMAGEWIDTH;
    fmt.fmt.pix.height = IMAGEHEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //*************************V4L2_PIX_FMT_YUYV****************
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == -1){
        printf("Setting Pixel Format error\n");
        return NULL;
    }
    if(ioctl(fd,VIDIOC_G_FMT,&fmt) == -1){
        printf("Unable to get format\n");
        return NULL;
    }

    uchar *buffer;
    struct v4l2_buffer buf;
    struct v4l2_requestbuffers req;

    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1)
    {
        printf("Requesting Buffer_1 error\n");
        return NULL;
    }
    //5 mmap for buffers
    buffer = (uchar *)malloc(req.count * sizeof(*buffer));
    if(!buffer){
        printf("Out of memory\n");
        return NULL;
    }
    unsigned int n_buffers;
    for(n_buffers = 0;n_buffers < req.count; n_buffers++){
        //struct v4l2_buffer buf = {0};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        if(ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1){
            printf("Querying Buffers_1 error\n");
            return NULL;
        }

        buffer = (uchar*)mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

        if(buffer == MAP_FAILED){
            printf("buffer_1 map error\n");
            return NULL;
        }

        printf("Length: %d\nAddress: %p\n", buf.length, buffer);
        printf("Image Length: %d\n", buf.bytesused);
    }
    //6 queue
    for(n_buffers = 0;n_buffers <req.count;n_buffers++){
        buf.index = n_buffers;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if(ioctl(fd,VIDIOC_QBUF,&buf)){
            printf("query buffer_1 error\n");
            return NULL;
        }
    }
    //7 starting
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd,VIDIOC_STREAMON,&type) == -1){
        printf("stream on error\n");return NULL;
    }

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    int lef,top;
    int min_width = IMAGEWIDTH/3;
    int min_height = IMAGEHEIGHT/2;
    Mat temp;

    char photo_num = 1;
    char photoName[20];
    time_t photo_time;
    char num = 1;
    struct tm *t1,*t2;
    unsigned t2_time,t1_time;
    time(&photo_time);
    t1 = localtime(&photo_time);
    char file_num = i + 1;
    t1_time =t1->tm_hour*3600 + t1->tm_min*60 + t1->tm_sec;
   // printf("%d:%d:%d\n",t1->tm_hour, t1->tm_min, t1->tm_sec);

    for(;;){
        ioctl(fd,VIDIOC_DQBUF,&buf);
        buf.index = 0;

#if 0
/***************** MJPEG -> Mat *******************/
        Mat M(IMAGEHEIGHT,IMAGEWIDTH,CV_8U);
        uchar *data = (uchar *)M.data;

        for(int k = 0;k < IMAGEHEIGHT*IMAGEWIDTH;k++){
            data[k] = *((uchar *)buffer + k);
        }

        Mat img = imdecode(M,IMREAD_GRAYSCALE);
#endif
/***************** MJPEG -> Mat *******************/
#if 1
/***************** MJPEG -> Mat *******************/
        Mat M(IMAGEHEIGHT,IMAGEWIDTH,CV_8UC3);
        uchar *data = (uchar *)M.data;

        for(int k = 0;k < IMAGEHEIGHT*IMAGEWIDTH;k++){
            data[k] = *((uchar *)buffer + k);
        }

        Mat img = imdecode(M,IMREAD_COLOR);
/***************** MJPEG -> Mat *******************/
#endif
        resize(img,temp,Size(min_width,min_height));
        top = i/3 *min_height;
        lef = i%3 *min_width;
        temp.copyTo(imageShow(Rect(lef,top,min_width,min_height)));

#if 1
        time(&photo_time);
        t2 = localtime(&photo_time);
        t2_time =t2->tm_hour*3600 + t2->tm_min*60 + t2->tm_sec;
       // printf("%d:%d:%d\n",t2->tm_hour, t2->tm_min, t2->tm_sec);

        if((t2_time - t1_time)  == num){
            t1_time = t2_time;
            sprintf(photoName,"./camera_%d/photo_%d.jpg",file_num,photo_num);
            photo_num++;
            imwrite(photoName,img);
        }
#endif

        ioctl(fd,VIDIOC_QBUF,&buf);
    }

    ioctl(fd,VIDIOC_STREAMOFF,&type);
}

int main(void)
{
    int i = 0;
    int a[] = {0,1,2,3,4,5};
    pthread_t tid[6];

    for(;i < sizeof(camera_device)/sizeof(camera_device[0]);i ++){
        if(pthread_create(&tid[i],NULL,show_camera,(void *)&a[i]) != 0){
            perror("pthread ctreate fail !\n");
            exit(EXIT_FAILURE);
        }
        pthread_detach(tid[i]);
    }

#if 1
    printf("Camera display~~~~~~\n");

    namedWindow("Camera",CV_WINDOW_AUTOSIZE);

    for(;;){
        imshow("Camera",imageShow);
        if((waitKey(1)&255) == 27)    exit(0);
    }
#endif
    return 0;
}
