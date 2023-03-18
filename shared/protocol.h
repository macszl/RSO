//
// Created by maciek on 3/18/23.
//

#ifndef RSO_1_PROTOCOL_H
#define RSO_1_PROTOCOL_H


// Define the request structures
struct base_request_t {
    char type[4];
    int RQ_id;
};

struct root_request_t {
    struct base_request_t header;
    double value;
};

struct root_response_t {
    struct base_request_t header;
    double rooted_value;
};

struct date_response_t {
    struct base_request_t header;
    int length;
    char * message;
};

#endif //RSO_1_PROTOCOL_H
