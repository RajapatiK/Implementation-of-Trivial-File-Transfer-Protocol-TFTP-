#include "tftp.h"
#include<string.h>
#include <stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<errno.h>

extern int fd;
tftp_packet packet;
char buffer[512];
int bytes_read,i;



void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode);


void send_file(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, char *filename)
{
    int index = 0;
    while ((bytes_read = read(fd, &buffer[index], 1)) > 0) 
    {
	index++;
        if (index == 512)
        {
	    packet.body.data_packet.block_number = ++i;
            packet.body.data_packet.size = index;  
	    memcpy(packet.body.data_packet.data, buffer, 512);
            send_request(sockfd, client_addr, filename, 3);
	    index = 0;
	}
    }
    if (index > 0)
    {
	packet.body.data_packet.block_number = ++i;
        packet.body.data_packet.size = index;
        memset(&buffer[index], 0, 512 - index);
        memcpy(packet.body.data_packet.data, buffer, 512);
        send_request(sockfd, client_addr, filename, 3);
        send_request(sockfd, client_addr, filename, 4);
    }	
}

void send_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    int server_len = sizeof(server_addr); 
    if(opcode == 2)
    {
	packet.opcode = htons(opcode);
	strcpy(packet.body.request.filename,filename);
	strcpy(packet.body.request.mode,"Netascii");
	sendto(sockfd, &packet, sizeof(packet), 0,(struct sockaddr *)&server_addr, sizeof(server_addr));
	int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
	if (n < 0) {
	    perror("Receive failed or timeout occurred");
	    return;
	} 
	else 
	{
	    if(ntohs(packet.opcode) == 4)
		send_file(sockfd, server_addr, server_len, filename);
	    else
		printf("Unexpected packet received. Opcode: %d\n", ntohs(packet.opcode));
	}
    }
    else if(opcode == 3)
    {
	packet.opcode = htons(opcode);
	packet.body.data_packet.flag = htons(0);
	sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, server_len);
	int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
	if (n < 0) {
	    perror("Receive failed or timeout occurred");
	    return;
	} 
	else 
	{
	    if(ntohs(packet.opcode) == 4)
		send_file(sockfd, server_addr, server_len, filename);
	    else
		printf("Unexpected packet received. Opcode: %d\n", ntohs(packet.opcode));
	}
    }
    else if(opcode == 4)
    {
	packet.opcode = htons(opcode);
	packet.body.ack_packet.block_number = 0;
	sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, server_len);
    }
}
void receive_request(int sockfd, struct sockaddr_in server_addr, char *filename, int opcode)
{
    int server_len = sizeof(server_addr);
    if(opcode == 1)
    {
	packet.opcode = htons(opcode);
	strcpy(packet.body.request.filename,filename);
	strcpy(packet.body.request.mode,"Netascii");
	sendto(sockfd, &packet, sizeof(packet), 0,(struct sockaddr *)&server_addr, sizeof(server_addr));
	int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
	if (n < 0) {
	    perror("Receive failed or timeout occurred");
	    return;
	}
	else
	{

	    if(ntohs(packet.opcode) == 4)
	    {
		if(ntohs(packet.body.ack_packet.block_number) == 1)
		{
		    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		     if(fd != -1)
		     {
	                 packet.opcode = htons(3);
			 sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, server_len);
			 int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
			 if(n<0)
			     return;
	                 receive_file(sockfd, server_addr, server_len, filename);
		     }
		     else
			 return;
		}
	    }
	    else
		printf("Unexpected packet received. Opcode: %d\n", ntohs(packet.opcode));
	}
    }
    else if(opcode == 3)
    {
	packet.opcode = htons(4);
	sendto(sockfd, &packet, sizeof(packet), 0, (struct sockaddr *)&server_addr, server_len);
	int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &server_len);
	if(n<0)
	    return;
	if(ntohs(packet.opcode) == 4)
	{
	    if(packet.body.ack_packet.block_number == 5)
	    {
		close(fd);
		printf("File Closed\n");
	    }
	}
	else
	    receive_file(sockfd, server_addr, server_len, filename);
    }

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
    packet.body.ack_packet.block_number = htons(2);
    receive_request(sockfd,client_addr,filename,3);    
}


