#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <fcntl.h>
#include <linux/fs.h>


#define PORT "9000"
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024
#define BACKLOG 10
int sockfd = -1;
int newsockfd = -1;
FILE *file = NULL;

void sig_handler(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        syslog(LOG_INFO, "Caught signal, exiting");
        if (sockfd != -1)
            close(sockfd);
        if (newsockfd != -1)
            close(newsockfd);
        if (file != NULL)
            fclose(file);
        remove(DATA_FILE);
        closelog();
        exit(0);
    }
}

int run_as_daemon()
{
    pid_t pid = fork();

    if(pid < 0)
    {
        perror("fork");
        return 1;
    }

    if(pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    if(setsid() < 0)
    {
        perror("setsid");
        return 1;
    }

    if(chdir("/") == -1)
    {
        return 1;
    }

    int i;
    for(i  = 0; i < sysconf(_SC_OPEN_MAX); i++)
    {
        close(i);
    }

    open("/dev/null",O_RDWR);
    dup(0);
    dup(0);

    return 0;
    
}

int main(int argc, char *argv[])
{

    bool become_daemon = false;

    if((argc == 2) && (strcmp(argv[1],"-d")) == 0)
    {
        printf("\nBecome daemon\n");
        become_daemon = true;
    }
    

    openlog(NULL, 0, LOG_USER);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    // set up addrinfo hints and node
    struct addrinfo hints, *servinfo, *p;
    int opt = 1;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)
    {
        syslog(LOG_ERR, "Getaddrinfo failed");
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            continue;
        }

        if(setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            syslog(LOG_ERR, "Setsockopt failed");
            close(sockfd);
            freeaddrinfo(servinfo);
            return -1;
        }

        if(bind(sockfd,p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            continue;
        }

        if(become_daemon)
        {
            printf("\nBecome daemon function\n");

            //if(run_as_daemon)
            //{
            //    perror("daemon");
            //}

            if(daemon(0,0))
               perror("daemon");
        }
         
        break;

    }

    freeaddrinfo(servinfo);

    if(p == NULL)
    {
        syslog(LOG_ERR, "Failed to bind");
        return -1;
    }

    if(listen(sockfd, BACKLOG) == -1)
    {
        syslog(LOG_ERR, "Listen failed");
        close(sockfd);
        return -1;
    }

    while (1)
    {

        struct sockaddr_storage cli_addr;

        socklen_t clilen = sizeof(cli_addr);

        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        if(newsockfd == -1)
        {
            syslog(LOG_ERR,"Accept failed");
            continue;
        }

        char client_ip[INET6_ADDRSTRLEN];

        void *addr;

        if (cli_addr.ss_family == AF_INET)
        {
            struct sockaddr_in *s = (struct sockaddr_in *)&cli_addr;
            addr = &(s->sin_addr);
        }
        else
        {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&cli_addr;
            addr = &(s->sin6_addr);
        }

        inet_ntop(cli_addr.ss_family, addr, client_ip, sizeof(client_ip));

        printf("Accepted connection from %s\n", client_ip);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        if((file = fopen(DATA_FILE, "a+")) == NULL)
        {
            syslog(LOG_ERR,"File open failed");
            close(newsockfd);
            continue;
        }

        ////////////////////////////
        // Receive packets and split at newline character
        ////////////////////////////

        char buffer[BUFFER_SIZE];
        char *accumulated_data = NULL;
        size_t accumulated_size = 0;

        ssize_t bytes_received = BUFFER_SIZE;

        char *data = NULL;
        size_t data_len = 0;

        while ((bytes_received = recv(newsockfd, buffer, BUFFER_SIZE, 0)) > 0)
        {
            data = realloc(data, data_len + bytes_received);
            if(data == NULL)
            {
                syslog(LOG_ERR, "Memory allocation failed");
                break;
            }

            memcpy(data+data_len, buffer, bytes_received);
            data_len += bytes_received;

            if(memchr(buffer,'\n', bytes_received) != NULL)
            {
                fwrite(data, 1, data_len, file);
                fflush(file);
                fseek(file,0,SEEK_SET);

                while((bytes_received = fread(buffer,1,BUFFER_SIZE,file))>0)
                {
                    send(newsockfd,buffer,bytes_received,0);
                }

                free(data);
                data = NULL;
                data_len = 0;

            }

        }
        free(data);
        fclose(file);
        file = NULL;
        close(newsockfd);
        newsockfd = -1;
        syslog(LOG_INFO, "Closed connection from %s", client_ip);

    }

    return 0;
}
