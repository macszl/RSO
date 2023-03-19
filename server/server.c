//
// Created by maciek on 3/18/23.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "../shared/protocol.h"
#include <errno.h>

void * handle_root_request(void * client_sockfd_ptr);
void * handle_date_request(void * client_sockfd_ptr);

struct thread_vars_t {
    struct base_request_t * baseRequest;
    struct root_request_t * rootRequest;
    int * client_sockfd_ptr;
};

struct thread_vars_t *create_thread_vars(int client_sockfd, struct base_request_t request_type, struct root_request_t * root_p) {
    struct thread_vars_t *thread_vars = malloc(sizeof(struct thread_vars_t));
    thread_vars->client_sockfd_ptr = malloc(sizeof(int));
    *(thread_vars->client_sockfd_ptr) = client_sockfd;
    thread_vars->baseRequest = malloc(sizeof(struct base_request_t));
    strncpy(thread_vars->baseRequest->type, request_type.type, 4);
    thread_vars->baseRequest->RQ_id = request_type.RQ_id;

    if(root_p != NULL) {
        thread_vars->rootRequest = malloc(sizeof(struct root_request_t));
        thread_vars->rootRequest->value = root_p->value;
    }
    return thread_vars;
}

void free_thread_vars(struct thread_vars_t *thread_vars) {
    free(thread_vars->client_sockfd_ptr);
    free(thread_vars->baseRequest);
    free(thread_vars);
}

void sigpipe_handler(int signum) {
    // Do nothing, just ignore the signal
}

pthread_mutex_t socket_mutex = PTHREAD_MUTEX_INITIALIZER;

int
main ()
{
    int server_sockfd, client_sockfd;
    socklen_t server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigpipe_handler;
    sigaction(SIGPIPE, &sa, NULL);

    server_sockfd = socket (AF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("setsockopt");
        exit(-1);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl (INADDR_ANY);
    server_address.sin_port = htons (9734);

    server_len = sizeof (server_address);
    int bind_return_val = bind (server_sockfd, (struct sockaddr *) &server_address, server_len);
    if( bind_return_val == -1 )
    {
        perror("Bind problem has happened");
        exit(-1);
    }
    /*  Create a connection queue, ignore child exit details and wait for clients.  */

    listen (server_sockfd, 5);

    signal (SIGCHLD, SIG_IGN);

    printf("[MAIN]Starting the main program loop\n");
    while (1)
    {
        printf ("[MAIN]Server trying to accept new connection.\n");

        /*  Accept connection.  */
        client_len = sizeof (client_address);
        client_sockfd = accept (server_sockfd,
                                (struct sockaddr *) &client_address,
                                &client_len);
        /*  Fork to create a process for this client and perform a test to see
            whether we're the parent or the child.  */
        printf ("[MAIN]Server has accepted new connection. Forking...\n");
        if (fork () == 0)
        {

            while (1)
            {
                // read the request type from the client
                struct base_request_t request_type;
                printf("%s" , "[MAIN] Reading request in a subordinate process...\n");


                ssize_t bytes_read =  read (client_sockfd, &request_type, sizeof(request_type));


                printf("[MAIN] Read bytes %ld\n", bytes_read);

                printf("[MAIN]Request type: %d\n", (int) request_type.type[3]);
                if (bytes_read == 0 || (bytes_read < 0 && errno == EPIPE) || request_type.type[3] == 3)
                {
                    printf("%s", "Quit, or error\n");
                    break;
                }
                else if (bytes_read < 0)
                {
                    perror("Error reading from socket");
                    break;
                }
                else if (request_type.type[3] == 2)
                {
                    printf("%s", "[MAIN]Requested date\n");
                    pthread_t date_thread;
                    struct thread_vars_t *thread_vars = create_thread_vars(client_sockfd, request_type, NULL);

                    pthread_create(&date_thread, NULL, handle_date_request, thread_vars);

                    pthread_detach(date_thread);
                    printf("%s", "[MAIN]Detached new date thread\n");
                }
                else if (request_type.type[3] == 1)
                {
                    printf("%s", "[MAIN]Requested rooted number\n");
                    pthread_t root_thread;

                    struct root_request_t root_request;
                    printf("[MAIN THREAD]: Waiting for value\n");

                    if (read(client_sockfd, &root_request, sizeof(root_request)) < 0) {
                        perror("Error reading root request");
                        exit(1);
                    }

                    struct thread_vars_t *thread_vars = create_thread_vars(client_sockfd, request_type, &root_request);

                    pthread_create(&root_thread, NULL, handle_root_request, thread_vars);

                    pthread_detach(root_thread);
                    printf("%s", "[MAIN]Detached new root thread\n");
                }

            }

            printf("%s", "[MAIN] Closing the socket\n");
            close (client_sockfd);

            pthread_mutex_destroy(&socket_mutex);

            exit (0);
        }

            /*  Otherwise, we must be the parent and our work for this client is finished.  */

        else
        {
            printf("%s", "[MAIN]Our work for this client is finished. Closing the socket\n");
            close (client_sockfd);
        }
    }
}


void * handle_root_request(void * void_thread_vars) {
    printf("[ROOT THREAD]: Entering thread\n");
    struct thread_vars_t thread_vars = *((struct thread_vars_t *) void_thread_vars);
    int client_sockfd = *(thread_vars.client_sockfd_ptr);
    struct base_request_t base_request = *(thread_vars.baseRequest);
    struct root_request_t root_request = *(thread_vars.rootRequest);

    free_thread_vars( (struct thread_vars_t *)void_thread_vars);

    printf("[ROOT THREAD]: Received value %f\n", root_request.value);
    struct root_response_t root_response;
    root_response.base_request.RQ_id = base_request.RQ_id;
    char bytes[] = { 0x01, 0x00, 0x00, 0x01 };
    strncpy(root_response.base_request.type, bytes, 4);
    root_response.rooted_value = sqrt(root_request.value);

    printf("[ROOT THREAD]: Sending value %f\n", root_response.rooted_value);

    pthread_mutex_lock(&socket_mutex);
    if (send(client_sockfd, &root_response, sizeof(root_response), 0) < 0) {
        perror("Error writing root response");
        pthread_mutex_unlock(&socket_mutex);
        exit(1);
    }
    pthread_mutex_unlock(&socket_mutex);

    printf("[ROOT THREAD]:Lock released\n");
    return NULL;
}
void * handle_date_request(void * void_thread_vars)
{
    printf("[DATE THREAD] Entering thread\n");
    struct thread_vars_t thread_vars = *((struct thread_vars_t *) void_thread_vars);
    int client_sockfd = *(thread_vars.client_sockfd_ptr);
    struct base_request_t base_request = *(thread_vars.baseRequest);

    free_thread_vars( (struct thread_vars_t *)void_thread_vars);

    time_t current_time;
    time(&current_time);
    char * time_string = ctime(&current_time);
    int time_string_length = strlen(time_string);

    struct date_response_t date_response;
    char bytes[] = { 0x01, 0x00, 0x00, 0x02 };
    strncpy(date_response.base_request.type, bytes, 4);
    date_response.base_request.RQ_id = base_request.RQ_id;

    date_response.length = time_string_length;
    date_response.message = malloc((time_string_length + 1) * sizeof(char));
    strncpy(date_response.message, time_string, time_string_length);
    date_response.message[time_string_length] = '\0';

    pthread_mutex_lock(&socket_mutex);
    write(client_sockfd, &date_response, sizeof(date_response.base_request) + sizeof(date_response.length));
    write( client_sockfd, date_response.message, date_response.length);
    pthread_mutex_unlock(&socket_mutex);


    free(date_response.message);
    printf("[DATE THREAD] Leaving\n");
    return NULL;
}