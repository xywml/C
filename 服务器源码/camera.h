/*========================================================================
	> File Name: camera.h
	> Author:  
	> Mail: 1216951938@qq.com 
	> Created Time: 2020年02月28日 星期五 19时31分09秒
==========================================================================*/

#ifndef _CAMERA_H
#define _CAMERA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define   MJPEG   1  //代表设置图片格式为jpeg成功
#define   NJPEG   0  //代表设置图片格式为jpeg失败

/*该结构体用于记录图片真实的格式信息*/
struct Camera_Info {
    unsigned int width; //图片宽度
    unsigned int height; //图片高度
    unsigned int pic_size; //图片大小
    unsigned int pic_format; //图片格式
};

/*该结构体用于存储要发送给客户端的图片信息*/
struct Camera_Pic {
    unsigned int size; //记录图片的大小
    char buf[1024*1024]; //记录图片的信息
};

int Camera_Init(char *pathname); //摄像头初始化函数

struct Camera_Pic *Camera_Getpic(struct Camera_Pic *pic); //获取一张照片函数

#endif
