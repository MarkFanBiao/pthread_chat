#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

//功能: 一定读到 len 个字节再返回
static inline int doRead(int sock, void *buf, int len)
{
    int already_read = 0;
    while(already_read != len)
    {
        int ret = read(sock, buf+already_read, len-already_read);
        if(ret > 0)
            already_read += ret;
        else
            return -1;
    }
    return 0;       //返回值是int
}

static inline int doWrite(int sock, const void *buf, int len)
{
    int already_write = 0;
    while(already_write != len)
    {
        int ret = write(sock, buf+already_write, len-already_write);
        if(ret > 0)
            already_write += ret;
        else
            return -1;
    }
    return 0;
}

//启动服务器, 封装函数
static inline int startTcpServer(unsigned short port, const char *ip, int backlog)
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int ret = bind(server, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0)
    {
        close(server);
        return -1;
    }

    listen(server, backlog);
    return server;
}

static inline int connectServer(unsigned short port, const char *ip)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0)
    {
        close(sock);
        return -1;
    }
    return sock;
}

//获取文件长度 封装
static inline int getFileLen(const char *filename)
{
    struct stat stbuf;
    int ret = stat(filename, &stbuf);
    if(ret < 0)
    {
        return -1;
    }
    return stbuf.st_size;
}

static inline void getPathAndFile(const char *filepath, char **dir, char **filename)
{
    //获取绝对目录，由于离开函数后，还要*dir和* filename使用realp，所以声明成静态的
    static char realp[1024];
    realpath(filepath, realp);
   //获取 目录部分 和 文件部分,由于目录 和 文件还要用于输出，所以用二级指针传进来
    *dir = realp;
    char *pos = rindex(realp, '/');
    *filename = pos+1;
    *pos = 0;
}







