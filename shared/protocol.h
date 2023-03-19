//
// Created by maciek on 3/18/23.
//

#ifndef RSO_1_PROTOCOL_H
#define RSO_1_PROTOCOL_H


// Define the request structures
struct base_request_t {
    char type[4];
    int RQ_id;
} __attribute__((packed));;

struct root_request_t {
    double value;
} __attribute__((packed));;

struct root_response_t {
    struct base_request_t base_request;
    double rooted_value;
} __attribute__((packed));;

struct date_response_t {
    struct base_request_t base_request;
    int length;
    char * message;
} __attribute__((packed));;

#endif //RSO_1_PROTOCOL_H
