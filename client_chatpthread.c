#include "./head.h"

int sock;       //设为全局 就可以多线程使用，不用传参

void sendUserName(const char *cmd, const char *name)
{
    char buf[8192];
    sprintf(buf, "%s|%s", cmd, name);

    int len = strlen(buf);
    char buflen[5];
    sprintf(buflen, "%04d", len);
    //分两次 发包
    doWrite(sock, buflen, 4);
    doWrite(sock, buf, len);
}

void sendMessage(const char *cmd, const char *msg, const char *toName)
{
    char buf[8192];
    sprintf(buf, "%s|%s|%s", cmd, toName, msg);

    int len = strlen(buf);
    char buflen[5];
    sprintf(buflen, "%04d", len);
    //分两次 发包
    doWrite(sock, buflen, 4);
    doWrite(sock, buf, len);
}

//子线程 回调函数   //负责接收 服务器消息
void *thread_func(void *arg)
{   
    char buf[8192];
    while(1)
    {   //服务器返回信息 规格 
        //9999ack|info          
        //9999msg|fromuser|content    
        buf[4] = 0;     //包都是分两次发，长度先发过来
        int ret = doRead(sock, buf, 4);
        //printf("ret = %d\n", ret);
        if(ret < 0)     //接收服务器的线程出问题了，就没必要继续了
        {
            exit(1);    
        }
        int len = atoi(buf);
        
        buf[len] = 0;
        doRead(sock, buf, len);     //ack|wang|nihao 或者 msg|liming|nihao

        char *saveptr = NULL;
        char *cmd = strtok_r(buf, "|", &saveptr);
        if(strcmp(cmd, "ack") == 0)     //ack|info
        {   //回复 提示信息
            char *info = strtok_r(NULL, "\0", &saveptr);
            printf("%s\n", info);
        }
        else if(strcmp(cmd, "msg") == 0)    //msg|fromUser|content
        {   //回复 消息来源包
            char *fromUser = strtok_r(NULL, "|", &saveptr);
            char *msg = strtok_r(NULL, "\0", &saveptr);
            printf("%s说: %s\n", fromUser, msg);
        }
    }
}


int main(int argc, char *argv[])
{
    sock = connectServer(9988, "127.0.0.1");
    //printf("sock = %d\n", sock);    
    //因为服务器有可能不回消息，这样接收就是阻塞的，所以需要分线程
    //一个线程负责 标准输入，另一个线程负责 接收服务器信息并拆包解析信息
    pthread_t tid;
    pthread_create(&tid, NULL, thread_func, NULL);  //sock是全局变量

    //主线程负责 从标准输入获取 信息，并把信息组包发送给服务器
    char buf[8192];
    while(1)
    {   //协议，规矩
        //1.键盘输名字  name|laowang
        //2.键盘输发送信息  msg|xiaoli|wocao
        fgets(buf, sizeof(buf), stdin);     //行输入  name|laowang 或  msg|xiaoli|wocao
        buf[strlen(buf)-1] = 0;     //去掉fgets或stdin里的 \n

        char *saveptr = NULL;
        char *cmd = strtok_r(buf, "|", &saveptr);
        if(strcmp(cmd, "name") == 0)        //name|laowang
        {   //输入的是 用户名字
            char *name = strtok_r(NULL, "\0", &saveptr);    //获取名字 laowang
            if(strlen(name) == 0)    //如果name| 后啥也没输
            {   //打印提示，并继续输入
                printf("name is NULL");
                continue;   
            }
            sendUserName(cmd,name);     //将 起名字信息 组包发送
        }
        else if(strcmp(cmd, "msg") == 0)    //msg|xiaoli|wocao
        {   //输入 发送包信息
            char *toName = strtok_r(NULL, "|", &saveptr);   // xiaoli|wocao
            if(strlen(toName) == 0)      //同样判断 啥也没输
            {
                printf("dest toName is NULL");
                continue;
            }
            char *msg = strtok_r(NULL, "\0", &saveptr); //wocao
            if(strlen(msg) == 0)
            {
                printf("msg is NULL");
                continue;
            }
            sendMessage(cmd, msg, toName);
        }
    }
    

	return 0;
}
