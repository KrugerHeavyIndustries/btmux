
#define NETWORK_HOSTNAME_MAX 1025
#define NETWORK_PORTNAME_MAX 33

struct descriptor_data {
    int fd; 
    char *remoteip;
    char *remotename;
    int remoteport;

    int flags;

}

    
