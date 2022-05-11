typedef struct rm
{
    char id_string[5];
    int id, port, is_primary, socket;
    struct sockaddr_in addr;
} rm;