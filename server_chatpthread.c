#include "./head.h"

//  strtok 不是线程安全函数(含有静态变量指向strtok上一次的位置), 多线程里 应该用 strtok_r

//声明 全局变量 保存所有已经连接的 用户
typedef struct User
{
    struct User *next;
    char name[32];
    int sock;   //记录连接后 服务器用来和 该用户通信的 新文件描述符
}User;

User *userptr = NULL;    //全局链表 记录用户
pthread_mutex_t mutex;  //声明互斥锁
#define LOCK(__mutex) pthread_mutex_lock(&__mutex)
#define UNLOCK(__mutex) pthread_mutex_unlock(&__mutex)

User *addUser(int sock)
{
    //先初始化一个用户结点
    User *user = malloc(sizeof(User));
    user->name[0] = 0;
    user->sock = sock;  //保存了 传进来的那个sock(accept后的新sock)
   //头插
    user->next = userptr;
    userptr = user;
    return user;
}

User *findUser(const char *name)
{
    User *user = userptr;
    while(user)     //user != NULL
    {
        if(strcmp(user->name, name) == 0)
            break;
        user = user ->next;
    }
    return user;
}

void delUser(User *user)
{
    User *node = userptr;
    if(node == user)
    {
        userptr = userptr->next;
        return;
    }
    while(node->next != user && node->next != NULL)
    {
        node = node->next;
    }
    if(node->next)
        node->next = node->next->next;
}

void transMsg(User *toUser, const char *msg, const char *fromUser)
{   //协议 9999msg|wang|hello
    //组包信息 msg|wang|hello
    char buf[8192];
    sprintf(buf, "msg|%s|%s", fromUser, msg);

    //包信息长度 9999
    int len = strlen(buf);
    char buflen[5];
    sprintf(buflen, "%04d", len);   //%04d 表示至少占4个字符

    //因为 这里用sprintf 无法处理 \0 的问题，所以分开发送
    doWrite(toUser->sock, buflen, 4);   //转发，是发包发包发包啊啊啊 啊啊
    doWrite(toUser->sock, buf, len);
}

void ackMsg(User *user, const char *info)
{   
    //协议 9999ack|content
    char buf[8192];
    sprintf(buf, "ack|%s", info);

    //包信息长度 9999
    int len = strlen(buf);
    char buflen[5];
    sprintf(buflen, "%04d", len);   //%04d 表示至少占4个字符

    //因为 这里用sprintf 无法处理 \0 的问题，所以分开发送
    doWrite(user->sock, buflen, 4);      //发包发包发包啊啊啊啊啊啊啊
    doWrite(user->sock, buf, len);
}

//子线程回调函数
void *thread_func(void *arg)
{
    User *user = (User *)arg;     //用user 将参数接过来
    char buf[8192];
    while(1)
    {
        //协议
        //1.起名字: 9999name|wang
        //2.发消息: 9999msg|liming|nihao
        
        // 读取 内容
        buf[4] = 0;
        int ret = doRead(user->sock, buf, 4);
        if(ret < 0)     //客户端已经关闭 socket
            break;

        int len = atoi(buf);
        buf[len] = 0;
        doRead(user->sock, buf, len);

        //解包  //strtok不是线程安全的，在多线程中使用strtok_r
        char *saveptr = NULL;
        char *cmd = strtok_r(buf, "|", &saveptr);
        if(strcmp(cmd, "name") == 0)
        {   //cmd 是 name，则需要起名字
            char *name  = strtok_r(NULL, "\0", &saveptr);
            printf("someone change name %s\n", name);
            LOCK(mutex);        //加锁 在锁内尽量完成一个完整功能
            //查找用户链表 判断是否有重名
            User *tmp = findUser(name);
            if(tmp == NULL)     //返回NULL 说明链表中没有 
            {
                strcpy(user->name, name);   //起名字
                ackMsg(user, "set name ok");
            }
            else    //说明 用户链表中已经有了
            {
                ackMsg(user, "duplicate name");
            }
            UNLOCK(mutex);
        }
        else if(strcmp(cmd, "msg") == 0)
        {   //cmd 是 msg  则是获取 信息接收者 和 信息内容
            char *toName = strtok_r(NULL, "|", &saveptr);
            char *msg = strtok_r(NULL, "\0", &saveptr);

            //知道了 是谁发的，要发给谁，内容是什么，就可以 负责转发了
            LOCK(mutex);
            //根据 信息，首先要找到目的用户 findUser
            User *toUser = findUser(toName);
            if(toUser)  //返回值toUser 存在，找到了
            {
                //转发消息规则  msg|wang|hello    //从wang处发来，内容为 hello
                transMsg(toUser, msg, user->name);
                ackMsg(user, "msg transferred");
            }
            else    //(没有找到该名字toName 用户)
            {   //回复一个 消息
                //信息 规格 9999ack|"消息"
                ackMsg(user, "user not exist");
            }
            UNLOCK(mutex);
        }
    }

    //ret < 0, break时 ，把该用户从userptr里删除，释放user内存
    LOCK(mutex);
    delUser(user);
    UNLOCK(mutex);
    free(user);
}

int main(int argc, char *argv[])
{
    int server = startTcpServer(9988, "0.0.0.0", 50);
    pthread_mutex_init(&mutex, NULL);
    
    //主线程不断接收
    //服务器结构 整个是一个while死循环，每次循环处理一个客服端连接
    while(1)
    {   //因为 accept 和 recv都是阻塞的，所以需要分线程
        int sock = accept(server, NULL, NULL);

        LOCK(mutex);
        //每次 接收数据成功，阻塞态打破，添加一个用户
        User *user = addUser(sock);
        UNLOCK(mutex);
        printf("some one connected\n");

        pthread_t tid;
        //本来是要将sock传过去，但是user已经把sock接了，用user就可以用到sock
        pthread_create(&tid, NULL, thread_func, user);  //把user传过去  
        pthread_detach(tid);
    }

	return 0;
}
