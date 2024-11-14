#include "tftp.h"
#include "tftp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define TIMEOUT_SEC 5

int fd;
int main() 
{
    tftp_client_t client;
    int num,flag = 0;
    while (num != 4) 
    {
	printf("Enter the option:\n1.Connect\n2.put\n3.Get\n4.Exit\n");
	scanf("%d",&num);
	switch (num)
	{
	    case 1:
		{
		    if(flag==0)
		    {
			printf("Connecting\nEnter the IP address: \n");
		        char serv_ip_buff[20];
			scanf(" %s",serv_ip_buff);
			flag = 1;
			connect_to_server(&client,serv_ip_buff,PORT);
		    }
		    break;
		}
	    case 2:
		{
		    if(flag == 1)
		    {
			printf("Enter the file name: \n");
		        char file_name[30];
		        scanf("%s",file_name);
			flag = 2;
                        put_file(&client,file_name);
		    }
		    else
			printf("Please connect the IP address by giving 1\n");
		    break;
		}
	    case 3:
		{
		    if(flag == 2)
		    {
			printf("Enter the file sent to server: ");
		        char file_name[30];
		        scanf("%s",file_name);
		        flag = 0;
		        get_file(&client,file_name);
		    }
		    else
			printf("Please connect the IP address by giving 1\n");
		    break;
		}
	    case 4:
		{
		    printf("Exit\n");
		    break;
		}
	    default:
		printf("Enter valid option\n");
	}
    }
}
// This function is to initialize socket with given server IP, no packets sent to server in this function
void connect_to_server(tftp_client_t *client, char *ip, int port) 
{
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    if((client->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
	printf("Error : Could not create socket\n");
	return;
    }
    if (setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("Error setting socket timeout");
        close(client->sockfd);
        return;
    }

    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(port);
    client->server_addr.sin_addr.s_addr = inet_addr(ip);
    client->server_len = sizeof(client->server_addr);
    strcpy(client->server_ip,ip);
}

void put_file(tftp_client_t *client,char *filename) 
{
    fd = open(filename, O_RDONLY);
    if(fd == -1)
	return;
    send_request(client->sockfd,client->server_addr,filename,2);
    close(fd);
    if (remove(filename) == 0) {
        printf("File '%s' deleted successfully.\n", filename);
    } else {
        perror("Error deleting file");
    }
        
}

void get_file(tftp_client_t *client, char *filename) 
{
    receive_request(client->sockfd,client->server_addr,filename,1);    
}

void disconnect(tftp_client_t *client) 
{
    close(client->sockfd);    
}
