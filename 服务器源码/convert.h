#ifndef __CONVERT_H__
#define __CONVERT_H__

void convert_yuv_to_rgb(const void *yuv, void *rgb, unsigned int width, unsigned int height, unsigned int bpp);
void convert_rgb_to_jpg_init(void);
int convert_rgb_to_jpg_work(void *rgb, void *jpeg, unsigned int width, unsigned int height, unsigned int bpp, int quality);
void convert_rgb_to_jpg_exit(void);

#endif
