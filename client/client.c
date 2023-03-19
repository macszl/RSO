/*  Make the necessary includes and set up the variables.  */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "../shared/protocol.h"


void * handle_sending_root( void * thread_args);
void * handle_getting_date( void * thread_args);
void * handle_quit( void * thread_args);

struct root_thread_vars_t {
    struct base_request_t * baseRequest;
    double * value;
    int * server_sockfd_ptr;
};

struct thread_vars_t {
    struct base_request_t * baseRequest;
    int * server_sockfd_ptr;
};


void free_root_thread_vars(struct root_thread_vars_t * root_thread_vars)
{
    free(root_thread_vars->server_sockfd_ptr);
    free(root_thread_vars->value);
    free(root_thread_vars->baseRequest);
    free(root_thread_vars);
}

void free_thread_vars(struct thread_vars_t *thread_vars) {
    free(thread_vars->server_sockfd_ptr);
    free(thread_vars->baseRequest);
    free(thread_vars);
}

struct thread_vars_t *create_thread_vars(int server_sockfd, struct base_request_t request_type) {
    struct thread_vars_t *thread_vars = malloc(sizeof(struct thread_vars_t));
    thread_vars->server_sockfd_ptr = malloc(sizeof(int));
    *(thread_vars->server_sockfd_ptr) = server_sockfd;
    thread_vars->baseRequest = malloc(sizeof(struct base_request_t));
    strncpy(thread_vars->baseRequest->type, request_type.type, 4);
    thread_vars->baseRequest->RQ_id = request_type.RQ_id;
    return thread_vars;
}

struct root_thread_vars_t *create_root_thread_vars(int server_sockfd, double value, struct base_request_t request_type) {
    struct root_thread_vars_t *root_thread_vars = malloc(sizeof(struct root_thread_vars_t));
    root_thread_vars->server_sockfd_ptr = malloc(sizeof(int));
    *(root_thread_vars->server_sockfd_ptr) = server_sockfd;
    root_thread_vars->baseRequest = malloc(sizeof(struct base_request_t));
    strncpy(root_thread_vars->baseRequest->type, request_type.type, 4);
    root_thread_vars->baseRequest->RQ_id = request_type.RQ_id;
    root_thread_vars->value = malloc(sizeof(double ));
    *(root_thread_vars->value) = value;
    return root_thread_vars;
}
pthread_mutex_t input_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
int
main ()
{
    int sockfd;
    socklen_t len;
    struct sockaddr_in address;
    int result;

    /*  Create a socket for the client.  */

    sockfd = socket (AF_INET, SOCK_STREAM, 0);

    /*  Name the socket, as agreed with the server.  */

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr ("127.0.0.1");
    address.sin_port = htons (9734);
    len = sizeof (address);

    /*  Now connect our socket to the server's socket.  */

    result = connect (sockfd, (struct sockaddr *) &address, len);

    if (result == -1)
    {
        perror ("oops: netclient");
        exit (1);
    }

    /*  We can now read/write via sockfd.  */

    char input[4];
    while (1)
    {
        printf("Enter request (1 to root number), (2 to get date), (3 to exit): \n");

        char input[3];
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';  // remove trailing newline if present
        pthread_t tid;
        if(strcmp( input, "1") == 0)
        {
            printf("Enter number to root\n");
            double number;
            scanf("%lf", &number);
            while (getchar() != '\n');

            struct base_request_t request_type = { {60,101,102,1},  getpid() };

            struct root_thread_vars_t * root_thread_vars = create_root_thread_vars(sockfd, number, request_type);
            printf("Sending double to root\n");
            if (pthread_create(&tid, NULL, handle_sending_root, (void *) root_thread_vars) != 0)
            {
                perror("Failed to create thread");
                exit(1);
            }
        }
        else if ( strcmp(input, "2") == 0 )
        {
            printf("Getting date\n");
            struct base_request_t request_type = { {60,102,103,2},  getpid() };

            struct thread_vars_t * thread_vars = create_thread_vars(sockfd, request_type);
            if (pthread_create(&tid, NULL, handle_getting_date, (void *) thread_vars) != 0)
            {
                perror("Failed to create thread");
                exit(1);
            }

        }
        else if(strcmp(input, "3") == 0)
        {
            printf("Quit\n");
            struct base_request_t request_type = { {64,101,102,3},  getpid() };

            struct thread_vars_t * thread_vars = create_thread_vars(sockfd, request_type);
            if (pthread_create(&tid, NULL, handle_quit, thread_vars) != 0)
            {
                perror("Failed to create thread");
                exit(1);
            }
            pthread_join(tid, NULL);
            break;
        }
        else
        {
            printf("Error\n");
            exit(1);
        }
        pthread_detach(tid);
    }

    pthread_mutex_destroy(&output_mutex);
    pthread_mutex_destroy(&input_mutex);
    close(sockfd);
    return 0;

}

void * handle_sending_root( void * void_thread_vars)
{
    struct root_thread_vars_t root_thread_vars = *((struct root_thread_vars_t *) void_thread_vars);
    int server_sockfd = *(root_thread_vars.server_sockfd_ptr);
    struct base_request_t base_request = *(root_thread_vars.baseRequest);
    double value = *(root_thread_vars.value);

    free_root_thread_vars( (struct root_thread_vars_t *)void_thread_vars);

    struct root_response_t root_response = { base_request, value};
    pthread_mutex_lock(&output_mutex);
    if (write(server_sockfd, &root_response, sizeof(root_response)) < 0) {
        perror("Error writing root request");
        pthread_mutex_unlock(&output_mutex);
        exit(1);
    }
    pthread_mutex_unlock(&output_mutex);

    pthread_mutex_lock(&input_mutex);
    if (read(server_sockfd, &root_response, sizeof(root_response)) < 0) {
        perror("Error reading root response");
        pthread_mutex_unlock(&input_mutex);
        exit(1);
    }
    pthread_mutex_unlock(&input_mutex);

    printf("Received root: %f\n", root_response.rooted_value);
    return NULL;
}

void * handle_getting_date( void * void_thread_vars)
{
    struct thread_vars_t thread_vars = *((struct thread_vars_t *) void_thread_vars);
    int server_sockfd = *(thread_vars.server_sockfd_ptr);
    struct base_request_t base_request = *(thread_vars.baseRequest);

    free_thread_vars( (struct thread_vars_t *)void_thread_vars);

    pthread_mutex_lock(&output_mutex);
    if (write(server_sockfd, &base_request, sizeof(base_request)) < 0) {
        perror("Error writing date request");
        pthread_mutex_unlock(&output_mutex);
        exit(1);
    }
    pthread_mutex_unlock(&output_mutex);

    struct date_response_t response;

    pthread_mutex_lock(&input_mutex);

    read(server_sockfd, &response.base_request.type, sizeof(response.base_request.type));
    read(server_sockfd, &response.base_request.RQ_id, sizeof(response.base_request.RQ_id));
    read(server_sockfd, &response.length, sizeof(response.length));

    response.message = (char*)malloc(response.length + 1);

    read(server_sockfd, response.message, response.length);

    response.message[response.length] = '\0';

    pthread_mutex_unlock(&input_mutex);

    for(int i = 0; response.message[i] != '\0'; i++)
    {
        printf("%c", response.message[i]);
    }
    printf("\n");
    return NULL;
}

void * handle_quit( void * void_thread_vars)
{
    struct thread_vars_t thread_vars = *((struct thread_vars_t *) void_thread_vars);
    int server_sockfd = *(thread_vars.server_sockfd_ptr);
    struct base_request_t base_request = *(thread_vars.baseRequest);

    free_thread_vars( (struct thread_vars_t *)void_thread_vars);

    pthread_mutex_lock(&output_mutex);
    if (write(server_sockfd, &base_request, sizeof(base_request)) < 0) {
        perror("Error writing quit request");
        pthread_mutex_unlock(&output_mutex);
        exit(1);
    }
    pthread_mutex_unlock(&output_mutex);
    
    return NULL;
}