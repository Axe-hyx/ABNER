#include <sys/socket.h>       // needed for socket()
#include <netinet/in.h>       // IPPROTO_RAW, IPPROTO_IP, IPPROTO_TCP, INET_ADDRSTRLEN
#include <linux/if_ether.h>
#include <stdio.h>
#include <errno.h>
//#include <android/log.h>

//#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
//#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))
int
main(int argc, char*argv[])
{
    int sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    printf("socket create : %d %d\n", sockfd, errno);
    return 0;
}
