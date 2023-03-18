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
#include "../shared/protocol.h"

int
main ()
{
    int server_sockfd, client_sockfd;
    socklen_t server_len, client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

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

    while (1)
    {
        char ch;

        printf ("server waiting\n");

        /*  Accept connection.  */

        client_len = sizeof (client_address);
        client_sockfd = accept (server_sockfd,
                                (struct sockaddr *) &client_address,
                                &client_len);

        /*  Fork to create a process for this client and perform a test to see
            whether we're the parent or the child.  */


        if (fork () == 0)
        {

            /*  If we're the child, we can now read/write to the client on client_sockfd.
                The five second delay is just for this demonstration.  */

            read (client_sockfd, &ch, 1);
            sleep (5);
            ch++;
            write (client_sockfd, &ch, 1);
            close (client_sockfd);
            exit (0);

            while (1)
            {
                // read the request type from the client
                struct base_request_t request_type;
                read (client_sockfd, &request_type, sizeof(request_type));

                if (request_type.type[3] == 3)
                {
                    // client requested quit, exit the loop
                    break;
                }
                else if (request_type.type[3] == 2)
                {
                    // client requested date, create a new thread to handle the task
                    pthread_t date_thread;
                    int *client_sockfd_ptr = malloc(sizeof(int));
                    *client_sockfd_ptr = client_sockfd;

                    pthread_create(&date_thread, NULL, handle_date_request, client_sockfd_ptr);
                }
                else if (request_type.type[3] == 1)
                {
                    // client requested root value, create a new thread to handle the task
                    pthread_t root_thread;
                    int *client_sockfd_ptr = malloc(sizeof(int));
                    *client_sockfd_ptr = client_sockfd;
                    pthread_create(&root_thread, NULL, handle_root_request, client_sockfd_ptr);
                }
            }

            close (client_sockfd);
            exit (0);
        }

            /*  Otherwise, we must be the parent and our work for this client is finished.  */

        else
        {
            close (client_sockfd);
        }
    }
}
