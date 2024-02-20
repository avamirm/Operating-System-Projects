#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <signal.h>

#define BUFFER_SIZE 255
#define ADS_NUMBER 100
#define SIZE 1024
#define BROADCAST "172.16.157.255"
#define NEGOTIATION_TIME 60
#define STATUS_SIZE 20
#define PRICE_SIZE 30
#define EXTENDED_BUF 2000
#define STD_OUT 1
int ads_count = 0;
char buffer[SIZE] = {0};
int ad_fd = -1;
int alarm_sig = 0;
char ad_name_on_neg[BUFFER_SIZE];
int is_on_neg = 0;
char write_buffer[2*SIZE] = {0};

struct Advertisement
{
    int port;
    char ad_name[BUFFER_SIZE];
    char seller_name[BUFFER_SIZE];
    char status[STATUS_SIZE];
};

struct Advertisement ads[ADS_NUMBER];

void  printAds()
{
    write(STD_OUT, "ads list:\n", strlen("ads list:\n"));
    for (int i = 0; i < ads_count; i++)
    {
        memset(write_buffer, '\0', strlen(write_buffer));
        sprintf(write_buffer, "Ad:%s:%s:%d:%s\n",ads[i].seller_name,ads[i].ad_name,ads[i].port,ads[i].status);
        write(STD_OUT,write_buffer, strlen(write_buffer) );
    }
} 
void addAdvertisement()
{
    struct Advertisement ad;
    int token_count = 0;
    int is_ad_changed = 0;
    memset(ad.status, '\0', sizeof(ad.status));
    memset(ad.ad_name, '\0', sizeof(ad.ad_name));
    memset(ad.seller_name, '\0', sizeof(ad.seller_name));
    char status[STATUS_SIZE];
    memset(status, '\0', sizeof(status));
    char * token = strtok(buffer, ":");
    char name[BUFFER_SIZE];
    memset(name, '\0', sizeof(name));
    while( token != NULL ) 
    {
        if(token_count == 1) 
            strcat(ad.seller_name ,token);
        else if(token_count == 2)
        {
            strcat(ad.ad_name ,token);
            strcat(name, token);
        }
        else if(token_count == 3)
            ad.port = atoi(token);
        else if(token_count == 4)
        {
            strcat(status, token);
        }
        token_count += 1;
        token = strtok(NULL, ":");
    }

    if(token_count == 5)
    {
        for(int i = 0;i < ads_count; i++)
            {
                if(!strcmp(ads[i].ad_name ,name))
                {
                    memset(ads[i].status, '\0', sizeof(ads[i].status));
                    strcat(ads[i].status, status);
                    is_ad_changed = 1;
                }
            }
    }
    if(is_ad_changed == 0)
    {
        strcat(ad.status , "waiting");
        ads[ads_count++] = ad;
    }
    printAds();
    
}

void alarmHandler()
{
    alarm_sig = 1; 
}
int connectServer(int port) 
{
    int fd;
    struct sockaddr_in server_address;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0)
        perror("socket error");
    server_address.sin_family = AF_INET; 
    server_address.sin_port = htons(port); 
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) 
        perror("connect error");
    

    return fd;
}

int main(int argc, char const *argv[]) 
{

    int bc_fd, new_socket, max_sd;

    fd_set master_set, working_set;

    if (argc < 2) 
    {
        memset(write_buffer, '\0', strlen(write_buffer));
        sprintf(write_buffer, "Error! no port provided\n");
        write(STD_OUT,write_buffer, strlen(write_buffer) );
        exit(1);
    }

    int buyer_port;
    buyer_port = atoi(argv[1]);
    
    bc_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (bc_fd < 0) 
    {
        perror("Error! opening server socket failed");
        exit(1);
    }

    int opt = 1, broadcast = 1;
    if(setsockopt(bc_fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast))< 0)
        perror("set socket");
    if(setsockopt(bc_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        perror("set socket");

    struct sockaddr_in buyer_addr;
    buyer_addr.sin_family = AF_INET;
    buyer_addr.sin_addr.s_addr = inet_addr(BROADCAST);
    buyer_addr.sin_port = htons(buyer_port);

    int bind_out = bind(bc_fd, (struct sockaddr *)&buyer_addr, sizeof(buyer_addr));

    if(bind_out < 0)
        perror("bind");
    char buyer_name[BUFFER_SIZE];
    write(1, "please enter your name as a buyer: ", 35);
    read(0, buyer_name, BUFFER_SIZE);
    buyer_name[strlen(buyer_name) - 1] = '\0';

    bzero(buffer, SIZE);

    FD_ZERO(&master_set);
    max_sd = bc_fd;
    FD_SET(bc_fd, &master_set);

    sigaction(SIGALRM, &(struct sigaction){.sa_handler = alarmHandler, .sa_flags = SA_RESTART}, NULL);                       

    FD_SET(0, &master_set);
    while (1) 
    {
        working_set = master_set;
        select(max_sd + 1, &working_set, NULL, NULL, NULL);
        if(alarm_sig)
        {
            memset(write_buffer, '\0', strlen(write_buffer));
            sprintf(write_buffer, "Negotiation time passed\n");
            write(STD_OUT,write_buffer, strlen(write_buffer) );

            alarm(0);
            char msg[400];
            sprintf(msg,"Time out:%s",ad_name_on_neg);
            send(ad_fd, msg, strlen(msg), 0);
            close(ad_fd);
            FD_CLR(ad_fd, &master_set); 
            alarm_sig = 0;
            is_on_neg = 0;
            memset(ad_name_on_neg, '\0', sizeof(ad_name_on_neg));
        }
        else if(FD_ISSET(0, &working_set))
        {
            memset(buffer, 0, SIZE);
            int buf_size = read(0, buffer, SIZE);
            buffer[strlen(buffer) - 1] = '\0';
            if(buffer[0] == 'O')
            {
                alarm_sig = 0;
                alarm(NEGOTIATION_TIME);

                is_on_neg = 1;
                int ad_port;
                char ad_port_str[6];
                memset(ad_port_str, 0, 6);
                int count = 0;
                char ad_name[PRICE_SIZE];
                char price[PRICE_SIZE];
                memset(ad_name, 0, PRICE_SIZE);
                memset(price, 0, PRICE_SIZE);
                char * token = strtok(buffer, ":");
                while( token != NULL ) 
                {
                    if(count == 1)
                        strcat(ad_name, token);
                    if(count == 2)
                    {
                        ad_port = atoi(token);
                        strcat(ad_port_str,token);
                    }
                    if(count == 3)
                        strcat(price,token);
                    token = strtok(NULL, ":");
                    count += 1;
                }

                ad_fd = connectServer(ad_port);

                FD_SET(ad_fd, &master_set);
                if (ad_fd > max_sd)
                    max_sd = ad_fd;
                char new_buf[3000];
                sprintf(new_buf, "Offer:%s:%s:%s:%s",buyer_name, ad_name,ad_port_str,price);
                memset(ad_name_on_neg, '\0', BUFFER_SIZE);
                strcat(ad_name_on_neg, ad_name);
                int send_ret = send(ad_fd, new_buf, strlen(new_buf), 0);
                memset(write_buffer, '\0', strlen(write_buffer));
                sprintf(write_buffer, "your offer has been sent\n");
                write(STD_OUT,write_buffer, strlen(write_buffer) );
            }

            else if(buffer[0] == 'N')
            {
                char new_buf[EXTENDED_BUF];
                memset(new_buf, 0, EXTENDED_BUF);
                sprintf(new_buf,"%s:%s", buyer_name, buffer);
                int send_ret = send(ad_fd, new_buf, strlen(new_buf), 0);
                if(send_ret < 0)
                    perror("send error");
                else
                {
                    memset(write_buffer, '\0', strlen(write_buffer));
                    sprintf(write_buffer, "your message was sent to seller successfully\n");
                    write(STD_OUT,write_buffer, strlen(write_buffer) );
                }
            }

        }


        else if(FD_ISSET(ad_fd, &working_set))
        {
            memset(buffer, 0, SIZE);
            if(recv(ad_fd, buffer, SIZE, 0) == 0)
            {
                close(ad_fd);
                FD_CLR(ad_fd, &master_set);
            }
            write(STD_OUT, buffer, strlen(buffer));    
            write(STD_OUT, "\n", 1);
            alarm_sig = 0;

            if(buffer[0] == 'R' || buffer[0] == 'C')
            {
                alarm(0);
                FD_CLR(ad_fd, &master_set);
                memset(ad_name_on_neg, '\0', strlen(ad_name_on_neg));
                is_on_neg = 0;
                char* token = strtok(buffer, ":");
                token = strtok(NULL, ":");
                for (int j = 0; j < ads_count; j++)
                {
                    if(!strcmp(token, ads[j].ad_name))
                    {
                        memset(ads[j].status, '\0', sizeof(ads[j].status));
                        if(buffer[0] == 'C')
                            strcat(ads[j].status,"selled");
                        else
                            strcat(ads[j].status,"waiting");
                    }
                }               
            }
            else if(buffer[0] == 'B')
            {
                FD_CLR(ad_fd, &master_set);
            }
            else
               alarm(NEGOTIATION_TIME);
        }
        if(FD_ISSET(bc_fd, &working_set))
        {
            memset(buffer, 0, SIZE);
            int rec_ret = recv(bc_fd, buffer, SIZE, 0);
            if(rec_ret < 0)
                perror("recieving error");
            char buf[EXTENDED_BUF];
            memset(buf, 0, EXTENDED_BUF);
            strcat(buf,buffer);
            char* token = strtok(buf, ":");
            if(!strcmp(token, "Time out") || !strcmp(token, "Reject"))
            {   
                token = strtok(NULL, ":");
                for (int j = 0; j < ads_count; j++)
                {
                    if(!strcmp(ads[j].ad_name, token))
                    {
                        memset(ads[j].status, '\0', strlen(ads[j].status));
                        strcat(ads[j].status, "waiting");
                    }
                }
                
                    printAds();
            }
            else if(!strcmp(token, "Confirm"))
            {   
                token = strtok(NULL, ":");
                for (int j = 0; j < ads_count; j++)
                {
                    if(!strcmp(ads[j].ad_name, token))
                    {
                        memset(ads[j].status, '\0', strlen(ads[j].status));
                        strcat(ads[j].status, "selled");
                    }
                }
                
                    printAds();
            }
            
            else
                addAdvertisement();
        }
    }
        
    return 0;
}