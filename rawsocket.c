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

#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/if.h>

#include <time.h>
#include <sys/time.h>

const char * interface = "wlan0";
//const char * interface = "enp2s0f1";


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

uint16_t
checksum (uint16_t *addr, int len)
{
    int nleft = len;
    int sum = 0;
    uint16_t *w = addr;
    uint16_t result = 0;

    while(nleft > 1){
        sum += *w++;
        nleft -=sizeof(uint16_t);
    }

    if(nleft == 1){
        *(uint8_t *)(&result) = *(uint8_t *)w;
        sum += result;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    result = ~sum;

    return (result);
}

void
mac_broadcast(int arg)
{

    int sock_raw_fd = (int)arg;

    unsigned char msg[1024] = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, //dst mac
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //src mac
        0x08, 0x00,                         //protocol field
        //IP header
        0x45, 0x00,                         //version header length and DSCP
        0x00, 0x25,                         //header length in bytes
        0x00, 0x00,                         //identification
        0x40, 0x00,                         //don't fragment offset
        0x40, 0x11,                         //TTL and protocol
        0x00, 0x00,                         //header checksum
        0x00, 0x00, 0x00, 0x00,             //32bit src IPV4 addr
        0xff, 0xff, 0xff, 0xff,             //32bit dst IPV4 addr
        // UDP header
        0x00, 0x00, 0x00, 0x00,             //src port and dst port
        0x00, 0x00, 0x00, 0x00              //packet length and checksum
    };
    struct ifreq req;

    snprintf(req.ifr_name, sizeof(req.ifr_name), "%s", interface);

    if(!(ioctl(sock_raw_fd, SIOCGIFADDR, &req)))
    {
        int num = ntohl(((struct sockaddr_in *) (&req.ifr_addr))->sin_addr.s_addr);
        int i;
        for(i = 0 ; i < 4; ++i)
        {
            msg[29 - i] = num>>8*i & 0xff;
        }
    }

    msg[34] = 6789 >> 8 & 0xff;
    msg[35] = 6789 & 0xff;
    msg[36] = 9999 >> 8 & 0xff;
    msg[37] = 9999 & 0xff;

    if(!ioctl(sock_raw_fd, SIOCGIFHWADDR, (char *) &req))
    {
        int i;
        for(i = 0; i < 6; ++i)
        {
            msg[6+i] = (unsigned char) req.ifr_hwaddr.sa_data[i];
        }
    }

    // fill in packaet checksum
    char buf[IP_MAXPACKET];
    char *ptr;
    int checksumlen = 0;

    ptr = &buf[0];
    memcpy(ptr, msg + 26, 4);
    ptr += 4;
    checksumlen += 4;

    memcpy(ptr, msg + 30, 4);
    ptr += 4;
    checksumlen += 4;

    *ptr = 0; ++ptr;
    checksumlen += 1;

    memcpy(ptr, msg+23 ,1);
    ++ptr;
    checksumlen +=1;

    memcpy(ptr, msg+38, 2);
    ptr += 2;
    checksumlen +=2;

    memcpy(ptr, msg+34, 4);
    ptr += 4;
    checksumlen +=4;

    memcpy(ptr, msg+38 ,2);
    ptr += 2;
    checksumlen +=2;

    *ptr = 0; ++ptr;
    *ptr = 0; ++ptr;
    checksumlen +=2;

    const char *data ="helloworld";
    int payloadlen = strlen(data);

    memcpy(ptr, data, payloadlen);
    checksumlen += payloadlen;

    int l;
    for(l = 0; l<payloadlen % 2;++l)
    {
        *ptr = 0;
        ++ptr;
        ++checksumlen;

    }

    payloadlen += payloadlen %2;
    int IPlen = payloadlen + 28;
    msg[16] = IPlen >> 8 & 0xff;
    msg[17] = IPlen & 0xff;

    uint16_t ip_sum = checksum((uint16_t *)(msg+14), 20);
    for(l = 0; l < 2; ++l){
        msg[25-l]  = ip_sum >> 8*l & 0xff;
    }

    uint16_t udpchecksum  = checksum((uint16_t *)buf, checksumlen);
    for(l = 0; l<2; ++l){
        msg[41-l] = udpchecksum >> 8*l & 0xff;
        msg[39-l] = (payloadlen + 8) >> 8*l & 0xff;
    }

    memcpy(msg + 42, data, payloadlen);

//    printf("%d\n",msg[payloadlen + 42] & 0xff); //don't know why printf 0%
//    printf("%d\n",msg[payloadlen + 42] );
    for(l = 0; l < payloadlen + 42 ;++l){
        printf("%02x ", msg[l]&0xff);
    }
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    if((sll.sll_ifindex = if_nametoindex(interface))==0)
    {
        printf("failed to get index");
        exit(1);
    }
    sll.sll_family = AF_PACKET;
    memcpy(sll.sll_addr, msg + 6 , 6 * sizeof(uint8_t));
    sll.sll_halen = 6;
    int n = sendto(sock_raw_fd, msg, payloadlen + 42, 0, (struct sockaddr *) &sll, sizeof(sll));
    if(n == -1)
        printf("send error %d\n", errno);
    else
        printf("send done  %d\n", n);

}
int
main(int argc, char **argv) {

    /*
     *test
     * */

    int sock, n;
    char buffer[2048];
    struct ethhdr *eth;
    struct iphdr *iph;

    if (0 > (sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP)))) {
        perror("socket");
        exit(1);
    }

    mac_broadcast(sock);
    return 1;

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
