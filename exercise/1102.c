#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

int main(void){
    uint16_t host_ip = 0x0400;
    uint16_t net_ip = htons(host_ip);
    // char *dot_dec = malloc(sizeof(char) * 40);
    // inet_ntop(AF_INET, &ip, dot_dec, 40);
    printf("%u\n", net_ip);
}