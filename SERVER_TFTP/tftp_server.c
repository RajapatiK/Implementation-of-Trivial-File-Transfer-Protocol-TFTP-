#include "tftp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


tftp_packet packet;
int fd;
void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet);

int main() 
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct timeval timeout;

    // Create UDP socket

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Error setting socket timeout");
        close(sockfd);
        return -1;
    }

    // Set socket timeout option
    //TODO Use setsockopt() to set timeout option


    // Set up server address

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    client_len = sizeof(struct sockaddr_in);

    // Bind the socket

    printf("TFTP Server listening on port %d...\n", PORT);

    // Main loop to handle incoming requests
    int n;
    while (1) 
    {
	int n = recvfrom(sockfd, &packet, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len);
	if (n < 0) 
	{
	    perror("Receive failed or timeout occurred");
	    return -1;
	}
	printf("Received opcode: %d\n", ntohs(packet.opcode));
	printf("Received request from client IP: %s, Port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

	handle_client(sockfd, client_addr, client_len, &packet);
    }
}


void handle_client(int sockfd, struct sockaddr_in client_addr, socklen_t client_len, tftp_packet *packet) 
{
    if (ntohs(packet->opcode) == 2)
    {
	fd = open(packet->body.request.filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if(fd != -1)
	{
	    packet->opcode = htons(4);  
	    packet->body.ack_packet.block_number = htons(0);
	    sendto(sockfd,packet,sizeof(*packet), 0, (struct sockaddr *)&client_addr, client_len);
	}
	else
	{
	    printf("error");
	    packet->body.error_packet.error_code = 1;
            strcpy(packet->body.error_packet.error_msg,"Unable to open the file");
	    sendto(sockfd,packet,sizeof(packet), 0,(struct sockaddr *)&client_addr, sizeof(client_addr));
	}

    }
    else if (ntohs(packet->opcode) == 3)
    {
	if(ntohs(packet->body.data_packet.flag==0))
	    receive_file(sockfd,client_addr,client_len,packet->body.request.filename);
	else
	    send_file(sockfd,client_addr,client_len,packet->body.request.filename);

    }
    else if (ntohs(packet->opcode) == 4)
    {
	if(packet->body.ack_packet.block_number == 0)
	{
	    printf("File closed\n");
	    close(fd);
	}
	else if(ntohs(packet->body.ack_packet.block_number) == 2)
	{
	    send_file(sockfd,client_addr,client_len,packet->body.request.filename);
	}
    }
    else if(ntohs(packet->opcode) == 1)
    {
	fd = open(packet->body.request.filename,O_RDONLY);
	if(fd != -1)
	{
	    packet->opcode = htons(4);  
	    packet->body.ack_packet.block_number = htons(1);
	    sendto(sockfd,packet,sizeof(*packet), 0, (struct sockaddr *)&client_addr, (client_len));
	}
	else
	{
	    printf("error");
	    packet->body.error_packet.error_code = 1;
            strcpy(packet->body.error_packet.error_msg,"Unable to open the file");
	    sendto(sockfd,packet,sizeof(packet), 0,(struct sockaddr *)&client_addr, sizeof(client_addr));
	}
    }

}

