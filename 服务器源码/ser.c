#include "camera.h"

static void handler()
{
    /*非阻塞回收子进程*/
    while( 0 < waitpid(-1, NULL, WNOHANG));
}

static int Camera_Server(int connfd)
{	
    int i = 0;
    /*
    for(i = 0; i < 8; i++)
    {
        struct Camera_Pic pic = {}; //用于存放照片信息的结构体
        Camera_Getpic(&pic); //获取张照片
    }
    */
    while(1)
    {
        struct Camera_Pic pic = {}; //用于存放照片信息的结构体
//	printf("%d - %s\n", __LINE__, __func__);
        Camera_Getpic(&pic); //获取张照片
//	printf("%d - %s\n", __LINE__, __func__);

        char pic_size[10] = {0}; //记录照片的大小
        snprintf(pic_size, sizeof(pic_size), "%u", pic.size); //将无符号的整形数大小转换为字符串
//	printf("pic.size = %d\n", pic.size);

        write(connfd, pic_size, sizeof(pic_size)); //将照片大小发送给客户端
        write(connfd, pic.buf, pic.size); //将图片的内容发送给客户端
    }
}

static int  Host_server(int port)
{
    /*1、创建网络套接字*/
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
    {
        printf("error: Host_server() -> socket\n");
        return -1;
    }
    printf("socket success\n");

    int on = 1;
    if(0 > setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) //设置端口复用
    {
        printf("error: Host_server() -> setsockopt\n");
        close(sockfd);
        return -1;
    }

    /*2、绑定本机地址和端口*/
    struct sockaddr_in server_addr; //存储服务器的信息
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    //server_addr.sin_addr.s_addr = inet_addr("0.0.0.0"); //0.0.0.0 代表本网络的所有主机
    server_addr.sin_addr.s_addr = INADDR_ANY; //0.0.0.0 代表本网络的所有主机

    int len = sizeof(server_addr);
    int ret = bind(sockfd, (struct sockaddr *)&server_addr, len);
    if(0 > ret)
    {
        printf("error: Host_server -> bind\n");
        close(sockfd);
        return -1;
    }
    printf("bind success\n");

    /*3、设置监听套接字*/
    ret = listen(sockfd, 20);
    if(0 > ret)
    {
        printf("error: Host_server -> listen\n");
        close(sockfd);
        return -1;
    }
    printf("listen success\n");
    
    signal(SIGCHLD, handler); // 子进程结束，父进程忽略信号，交给handler函数处理
    struct sockaddr_in client_addr; //接收客户端的信息
    int client_len = sizeof(client_len);
    pid_t pid; //进程ID
    int connfd; //通信套接子

    /*4、循环等待客户端连接，生成通行套接字*/
    while(1)
    {
        if( (connfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len)) < 0)
        {
            printf("error: Host_server -> accept\n");
            return -1;
        }
        printf("accept IP: %s port %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /*将摄像头服务交给子进程处理*/
        if((pid = fork()) < 0)
        {
            printf("error: Host_server -> fork\n");
            return -1;
        }else if(pid == 0) {
            Camera_Server(connfd); 
            return 0;
        }else {
            close(connfd);
        }
    }
    close(sockfd);
    return 0;
}

int main()
{
    int ret;

    /*初始化摄像头*/
    ret = Camera_Init("/dev/video0"); 
    if(ret == -1)
    {
        printf("error: main() -> Camera_Init\n");
        return -1;
    }
    /*初始化主机服务器*/
    ret = Host_server(8888);
    if(ret == -1)
    {
        printf("error: main() -> Host_server\n");
        return -1;
    }
    return 0;
}
