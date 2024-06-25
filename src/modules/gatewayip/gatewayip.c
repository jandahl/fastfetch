#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/route.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netinet/ip_icmp.h>

#define GATEWAY_IP_DISPLAY_NAME "Gateway IP"

static int getDefaultGateway(char* gateway_ip)
{
    FILE* fp;
    char line[256];
    char* iface;
    unsigned long dest, gw, mask, flags, refcnt, use, metric, mtu, win, irtt;

    if ((fp = fopen("/proc/net/route", "r")) == NULL)
    {
        perror("fopen");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (sscanf(line, "%ms %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx\n", &iface, &dest, &gw, &flags, &refcnt, &use, &metric, &mask, &mtu, &win, &irtt) == 11)
        {
            if (dest == 0)
            {
                struct in_addr addr;
                addr.s_addr = gw;
                strcpy(gateway_ip, inet_ntoa(addr));
                free(iface);
                fclose(fp);
                return 0;
            }
        }
        free(iface);
    }

    fclose(fp);
    return -1;
}

static unsigned short checksum(void* b, int len)
{
    unsigned short* buf = b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

static int sendPing(const char* ip)
{
    int sockfd;
    struct sockaddr_in addr;
    struct icmp icmp_pkt;
    char buffer[1024];
    struct timeval timeout = {1, 0}; // 1 second timeout

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
    {
        perror("socket");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip);

    memset(&icmp_pkt, 0, sizeof(icmp_pkt));
    icmp_pkt.icmp_type = ICMP_ECHO;
    icmp_pkt.icmp_code = 0;
    icmp_pkt.icmp_cksum = checksum(&icmp_pkt, sizeof(icmp_pkt));

    if (sendto(sockfd, &icmp_pkt, sizeof(icmp_pkt), 0, (struct sockaddr*)&addr, sizeof(addr)) <= 0)
    {
        perror("sendto");
        close(sockfd);
        return -1;
    }

    if (recvfrom(sockfd, buffer, sizeof(buffer), 0, NULL, NULL) <= 0)
    {
        perror("recvfrom");
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return 0; // Ping was successful
}

void printGatewayIp(bool ping)
{
    char gateway_ip[INET_ADDRSTRLEN];

    if (getDefaultGateway(gateway_ip) != 0)
    {
        printf("%s: Failed to determine gateway IP\n", GATEWAY_IP_DISPLAY_NAME);
        return;
    }

    printf("%s: %s", GATEWAY_IP_DISPLAY_NAME, gateway_ip);

    if (ping)
    {
        if (sendPing(gateway_ip) == 0)
            printf(" (Ping succeeded)\n");
        else
            printf(" (Ping failed)\n");
    }
    else
    {
        printf("\n");
    }
}

int main(int argc, char* argv[])
{
    bool ping = false;

    if (argc > 1 && strcmp(argv[1], "--ping") == 0)
    {
        ping = true;
    }

    printGatewayIp(ping);

    return 0;
}
