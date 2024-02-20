#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define BROADCAST "172.16.157.255"
#define BUFFER_SIZE 255
#define ADS_NUMBER 100
#define SIZE 1024
#define NAME_SIZE 20
#define EXTENDED_BUF 2000
#define STD_OUT 1

char buffer[SIZE] = {0};
char write_buffer[2*SIZE] = {0};

struct Advertisement
{
    int sock;
    int buyer_offer;
    char ad_name [BUFFER_SIZE];
    char status[NAME_SIZE];
    char price[NAME_SIZE];
    int port;
    int is_in_neg;
};

struct Advertisement ads[ADS_NUMBER];

int ads_count = 0;
int is_on_negotiation = 0;

int addAdvertisement()
{
    struct Advertisement ad;

    int token_count = 0;
    char * token = strtok(buffer, ":");
    memset(ad.ad_name, 0, BUFFER_SIZE);
    memset(ad.status, 0, NAME_SIZE);
    memset(ad.price, 0, NAME_SIZE);
    while( token != NULL ) 
    {
        if(token_count == 0) 
            strcat(ad.ad_name ,token);
        else if(token_count == 1) 
            ad.port = atoi(token);
        token_count += 1;
        token = strtok(NULL, ":");
    }
    strcat(ad.status, "waiting");

    sprintf( write_buffer, "You shared an advertisement with name: %s and port: %d\n", ad.ad_name, ad.port );
    write(STD_OUT, write_buffer, strlen(write_buffer));

    struct sockaddr_in address;
    int ad_fd;
    ad_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    int socket_out = setsockopt(ad_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if(socket_out < 0)
        perror("socket error");
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(ad.port);

   int bind_out =  bind(ad_fd, (struct sockaddr *)&address, sizeof(address));

   if(bind_out < 0)
        perror("bind");
    
    int listen_out = listen(ad_fd, 4);
    if(listen_out < 0 )
        perror("listen");
    ad.sock = ad_fd;
    ad.is_in_neg = 0;
    ad.buyer_offer = -1;
    ads[ads_count++] = ad;

    return ad_fd;
}
int acceptClient(int server_fd) 
{
    int client_fd;
    struct sockaddr_in client_address;
    int address_len = sizeof(client_address);
    client_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &address_len);
    if(client_fd < 0)
        perror("error in accepting");
    return client_fd;
}
int main(int argc, char const *argv[]) 
{
    int bc_fd, seller_port, broadcast = 1, opt = 1;
    struct sockaddr_in bc_addr;


    if (argc < 2)
    {
        memset(write_buffer, '\0', strlen(write_buffer));
        sprintf(write_buffer, "Error! no port provided\n");
        write(STD_OUT,write_buffer, strlen(write_buffer) );
        exit(1);
    }

    seller_port = atoi(argv[1]);

    bc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (bc_fd < 0) 
    {
        perror("Error! opening server socket failed");
        exit(1);
    }

    if (setsockopt(bc_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
        perror("setsockopt");
    if (setsockopt(bc_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        perror("setsockopt");
    bc_addr.sin_family = AF_INET;
    bc_addr.sin_addr.s_addr = inet_addr(BROADCAST);
    bc_addr.sin_port = htons(seller_port);

    if (bind(bc_fd, (struct sockaddr *)&bc_addr, sizeof(bc_addr)))
        perror("bind");
    

    char seller_name[BUFFER_SIZE];
    write(1, "please enter your name as a seller: ", 36);
    memset(seller_name, '\0', sizeof(seller_name));
    read(0, seller_name, BUFFER_SIZE);
    seller_name[strlen(seller_name)-1] = '\0';

    char file_name[300];
    memset(file_name, '\0', 300);
    sprintf(file_name, "%s.txt", seller_name);
    int text_file_fd = open(file_name, O_CREAT | O_RDWR | O_APPEND, 0644);


    bzero(buffer, SIZE);

    int max_sd, new_socket;
    fd_set master_set, working_set;
    FD_ZERO(&master_set);
    max_sd = 0;
    FD_SET(0, &master_set);
    while (1) 
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if(FD_ISSET(0, &working_set))
        {
            memset(buffer, 0, SIZE);
            read(0, buffer, SIZE);
            buffer[strlen(buffer) - 1] = '\0';
            
            if(buffer[0] == 'A')
            {
                is_on_negotiation = 0;
                for (int k = 0; k < ads_count; k++)
                {
                    if(!strcmp(ads[k].status, "on negotiation"))
                    {
                        is_on_negotiation = 1;
                        break;
                    }
                }
                
                if(is_on_negotiation == 0)
                {
                    char buf[SIZE];
                    memset(buf, '\0', sizeof(buf));
                    strncpy(buf, buffer +3, strlen(buffer)-3);
                    memset(buffer, '\0', sizeof(buffer));
                    strcat(buffer, buf);  
                         
                    int ad_fd = addAdvertisement();
                    FD_SET(ad_fd, &master_set);
                    if (ad_fd > max_sd)
                        max_sd = ad_fd;
                    char new_buf[EXTENDED_BUF];
                    memset(new_buf, '\0', sizeof(new_buf));
                    sprintf(new_buf, "Ad:%s:%s", seller_name, buf);
                    int a = sendto(bc_fd, new_buf, strlen(new_buf), 0,(struct sockaddr *)&bc_addr, sizeof(bc_addr));
                }
                else
                {
                    memset(write_buffer, '\0', strlen(write_buffer));
                    sprintf(write_buffer, "You can't add new advertisement!\n");
                    write(STD_OUT, write_buffer, strlen(write_buffer));
                }
            }
            else if(buffer[0] == 'N')
            {
                char * ad_name = strtok(buffer, ":");
                ad_name = strtok(NULL, ":");
                char *neg = strtok(NULL, ":");
                for (int i = 0; i < ads_count; i++)
                {
                    if(!strcmp(ads[i].ad_name , ad_name))
                    {
                        char msg[EXTENDED_BUF];
                        memset(msg, '\0', sizeof(msg));
                        sprintf(msg,"%s:%s",seller_name, neg);
                        int send_ret  = send(ads[i].buyer_offer, msg, strlen(msg), 0);
                        if(send_ret < 0)
                            perror("send");
                        else
                        {
                            memset(write_buffer, '\0', strlen(write_buffer));
                            sprintf(write_buffer, "your message was sent to buyer successfully\n");
                            write(STD_OUT, write_buffer, strlen(write_buffer));
                        }
                    }
                }
            }
            else if(buffer[0] == 'C')
            {
                char * ad_name = strtok(buffer, ":");
                ad_name = strtok(NULL, ":");
                for (int j = 0; j < ads_count; j++)
                {
                   if(!strcmp(ads[j].ad_name,ad_name))
                   {
                        memset(write_buffer, '\0', strlen(write_buffer));
                        sprintf(write_buffer,"%s %s\n", ads[j].ad_name, ads[j].price);
                        write(text_file_fd, write_buffer, strlen(write_buffer));

                        char confirm[100];
                        memset(ads[j].status, '\0', strlen(ads[j].status));
                        strcat(ads[j].status, "selled");
                        memset(confirm, '\0', 100);
                        sprintf(confirm, "Confirm:%s", ad_name);
                        int send_return = send(ads[j].buyer_offer, confirm, strlen(confirm), 0);
                        if(send_return > 0)
                        {
                              char* confirm_msg = "confirm message sent to buyer\n";
                              write(1, confirm_msg, strlen(confirm_msg));
                        }

                        else
                            perror("send");
                        
                        sendto(bc_fd, confirm, strlen(confirm), 0,(struct sockaddr *)&bc_addr, sizeof(bc_addr));
                        
                        close(ads[j].buyer_offer);
                        FD_CLR(ads[j].buyer_offer, &master_set); 
                        ads[j].buyer_offer = -1;
                        break;
                   }
                }    
                
            }
            else if(buffer[0] == 'R')
            {
                char * ad_name = strtok(buffer, ":");
                ad_name = strtok(NULL, ":");
                for (int j = 0; j < ads_count; j++)
                {
                   if(!strcmp(ads[j].ad_name,ad_name))
                   {
                        char rej[100];
                        memset(rej, '\0', sizeof(rej));
                        sprintf(rej, "Reject:%s", ad_name);
                        memset(ads[j].status, '\0', strlen(ads[j].status));
                        memset(ads[j].price, '\0', strlen(ads[j].price));
                        strcat(ads[j].status, "waiting");
                        int send_return = send(ads[j].buyer_offer, rej, strlen(rej), 0);
                        if(send_return > 0)
                        {
                            memset(write_buffer, '\0', strlen(write_buffer));
                            sprintf(write_buffer, "Reject message sent to buyer\n");
                            write(STD_OUT, write_buffer, strlen(write_buffer));
                        }  
                        else
                            perror("reject error");
                        sendto(bc_fd, rej, strlen(rej), 0,(struct sockaddr *)&bc_addr, sizeof(bc_addr));
                        close(ads[j].buyer_offer);
                        FD_CLR(ads[j].buyer_offer, &master_set); 
                        ads[j].buyer_offer = -1;
                        break;
                   }
                }    
            }    
        }
        for (int i = 1; i <= max_sd; i++) 
        {
            if (FD_ISSET(i, &working_set))
            {
                for(int j = 0; j < ads_count; j++)
                {
                    if(ads[j].sock == i)
                    {
                        new_socket = acceptClient(i);
                        if(!strcmp(ads[j].status , "waiting")) 
                        {
                            ads[j].buyer_offer = new_socket;
                            FD_SET(new_socket, &master_set);
                            if (new_socket > max_sd)
                                max_sd = new_socket;
                            memset(buffer, 0, SIZE);
                            int rcv_out = recv(new_socket , buffer, SIZE, 0);
                            if(rcv_out == 0)
                            {
                                close(i);
                                FD_CLR(i, &master_set);
                            }
                            if(rcv_out < 0)
                                perror("recieve error");
                            write(STD_OUT, buffer, strlen(buffer));
                            write(STD_OUT, "\n", 1);
                            int token_count = 0;
                            char * token = strtok(buffer, ":");
                            
                            while( token != NULL ) 
                            {
                                if(token_count == 4)
                                    strcat(ads[j].price ,token); 
                                token_count += 1;
                                token = strtok(NULL, ":");
                            }
                            memset(ads[j].status, 0, strlen(ads[j].status));
                            strcat(ads[j].status,"on negotiation");

                            ads[j].buyer_offer = new_socket;
                        
                            char new_buf[EXTENDED_BUF];
                            sprintf(new_buf,"%s recieved your offer", seller_name);
                            send(new_socket, new_buf, strlen(new_buf), 0);
                            char buff[EXTENDED_BUF] = {0};
                            sprintf(buff, "Ad:%s:%s:%d:on negotiation", seller_name,ads[j].ad_name,ads[j].port);
                            int a = sendto(bc_fd, buff, strlen(buff), 0,(struct sockaddr *)&bc_addr, sizeof(bc_addr));
                        }
                        else
                        {
                            send(new_socket, "Busy", strlen("Busy"), 0);
                            close(new_socket);
                        }
                        
                    }
                    else if(ads[j].buyer_offer == i)
                    {
                        memset(buffer, 0, SIZE);
                        if(recv(i , buffer, SIZE, 0)==0)
                        {
                            close(i);
                            FD_CLR(i, &master_set);
                        }
                        write(STD_OUT, buffer, strlen(buffer));
                        write(STD_OUT, "\n", 1);
                        char * token = strtok(buffer, ":");
                        if(!strcmp(token, "Time out"))
                        {
                            token = strtok(NULL, ":");
                            char new_buf [EXTENDED_BUF];
                            memset(new_buf, '\0', EXTENDED_BUF);
                            sprintf(new_buf,"Time out:%s", token);
                            FD_CLR(ads[j].buyer_offer, &master_set); 
                            ads[j].is_in_neg = 0;
                            sendto(bc_fd, new_buf, strlen(new_buf), 0,(struct sockaddr *)&bc_addr, sizeof(bc_addr));
                            ads[j].buyer_offer = -1;
                            memset(ads[j].status, '\0', strlen(ads[j].status));
                            memset(ads[j].price, '\0', strlen(ads[j].price));
                            strcat(ads[j].status, "waiting");
                        }
                        else
                        {
                            int token_count = 0;
                            int is_offer = 0;
                            while( token != NULL ) 
                            {
                                if(token_count == 1)
                                {
                                    if(!strcmp(token, "offer"))
                                    is_offer = 1;
                                }
                                if(token_count == 3)
                                {
                                    memset(ads[j].price, 0, strlen(ads[j].price));
                                    strcat(ads[j].price, token);
                                }
                                token_count += 1;
                                token = strtok(NULL, ":");
                            }
                        }
                    }
                }
            }
            
            
        }
    }
        
    return 0;
}
