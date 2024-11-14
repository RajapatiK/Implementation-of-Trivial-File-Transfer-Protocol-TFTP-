/* Common file for server & client */

#include "tftp.h"
#include<string.h>
#include <stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>


extern int fd;
extern tftp_packet packet;
char buffer[512];
int i = 0,bytes_read;

void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename) 
{
    int index = 0;
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        packet.body.data_packet.block_number = htons(++i);
        packet.body.data_packet.size = bytes_read;
        memcpy(packet.body.data_packet.data, buffer, bytes_read);
        packet.opcode = htons(3);

        if (sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Failed to send packet");
            return;
        }

        index = 0;  // Reset index for next buffer chunk
    }
    packet.opcode = htons(4); // ACK opcode to indicate end of transfer
    packet.body.ack_packet.block_number = htons(5);
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, client_len);
}

void receive_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename) 
{
    int ret = write(fd,packet.body.data_packet.data,packet.body.data_packet.size);
    if(ret == -1)
    {
	printf("Error\n");
        packet.body.ack_packet.block_number = htons(0); 
    }
    packet.opcode = htons(4);  
    packet.body.ack_packet.block_number = htons(1); 
    sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, client_len);
}


