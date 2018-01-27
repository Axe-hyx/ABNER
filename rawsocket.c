#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>

#include <time.h>
#include <sys/time.h>

const char *
inet_ntop(int family, const void *addrptr, void *strptr, size_t len) {
    const u_char *p = (const u_char *) addrptr;

    if (family == AF_INET) {
        char temp[INET_ADDRSTRLEN];
        snprintf(temp, sizeof(temp), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
        if (strlen(temp) >= len) {
            errno = ENOSPC;
            return (NULL);
        }
        strcpy(strptr, temp);
        return (strptr);
    }
    errno = EAFNOSUPPORT;
    return (NULL);
}

int
main(int argc, char **argv) {
    int sock, n;
    char buffer[2048];
    struct ethhdr *eth;
    struct iphdr *iph;

    if (0 > (sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP)))) {
        perror("socket");
        exit(1);
    }

    while (1) {
        n = recvfrom(sock, buffer, 2048, 0, NULL, NULL);
        // fprintf(stdout, "time : \n%u\n", (unsigned)time(NULL));


        int proto = (buffer + 14 + 9)[0];
	if(proto == IPPROTO_TCP)
		continue;

        printf("=====================================\n");
        /* Example of timestamp in microsecond. */
        struct timeval timer_usec;
        long long int timestamp_usec; /* timestamp in microsecond */
        if (!gettimeofday(&timer_usec, NULL)) {
            timestamp_usec = ((long long int) timer_usec.tv_sec) * 1000000 +
                             (long long int) timer_usec.tv_usec;
        } else {
            timestamp_usec = -1;
        }
        printf("%lld\b\b\b ms\n", timestamp_usec);
        printf("%d bytes read\n\n", n);

        //接收到的数据帧头6字节是目的MAC地址，紧接着6字节是源MAC地址。
        eth = (struct ethhdr *) buffer;
        printf("Dst MAC addr:	%02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_dest[0], eth->h_dest[1], eth->h_dest[2],
               eth->h_dest[3], eth->h_dest[4], eth->h_dest[5]);
        printf("Src MAC addr:	%02x:%02x:%02x:%02x:%02x:%02x\n", eth->h_source[0], eth->h_source[1], eth->h_source[2],
               eth->h_source[3], eth->h_source[4], eth->h_source[5]);

        iph = (struct iphdr *) (buffer + sizeof(struct ethhdr));

        char str[INET_ADDRSTRLEN];
	
 //       int proto = (buffer + 14 + 9)[0];
	
	printf("Protocol : ");
	switch(proto ){
		case IPPROTO_UDP:
		case IPPROTO_TCP:
			printf("  	%s\n", proto==IPPROTO_UDP ? "UDP": "TCP");
			break;
		case IPPROTO_RAW:
			printf("  	%s\n", "RAW");
			break;
		default:
			printf("	Other Protocol\n");
	}

        printf("Dst host :      %s\n", inet_ntop(AF_INET, &iph->daddr, str, sizeof(str)));
        printf("Src host :	%s\n", inet_ntop(AF_INET, &iph->saddr, str, sizeof(str)));
        if (iph->version == 4 && iph->ihl == 5) {
            //printf("Source host:%s\n",inet_ntoa(iph->saddr));
            //printf("Dest host:%s\n",inet_ntoa(iph->daddr));
        }

	printf("\n");
	char *p = buffer;
	int i = 14 + 20;
	if(proto == IPPROTO_TCP){
		i += 20;
	}else if(proto == IPPROTO_UDP){
		i += 6;
	}
	
	int j = 0;
	for(;i < n - 4; ++i,++j){
		printf("%02x ", *p & 0xff);
		if(j == 16 ){
			j = -1; //cafeful
			printf("\n");
		}
		if(j == 8){
			printf("  ");
		}
		++p;
	}
	printf("\n");
        //char *p = buffer + offset;
        //size_t i = offset;
        //for (; i < strlen(buffer); ++i) {
        //    printf("%c", *p++);
        //}

    }
}
