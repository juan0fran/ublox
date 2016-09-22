#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <poll.h>
#include <stdlib.h>

#include <cc_beacon_iface.h>

static int InitTCPClientSocket(char * ip_addr, char * port_num)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
    portno = atoi(port_num);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(ip_addr);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
    {
    	exit(0);
    }
    return sockfd;
}

static int InitUNIXClientSocket(char * sock_file)
{
	int fd;
	struct sockaddr_un addr;

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, sock_file, sizeof(addr.sun_path)-1);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    	perror("connect error");
    	exit(-1);
	}

	return fd;
}

int BeaconConnect (char * addr, char * port)
{
	return InitTCPClientSocket(addr, port);
}

void BeaconClose (int fd)
{
	close (fd);
}

/* Just to ensure is a 32 bits int -> int32_t */
int BeaconWrite (int fd, BYTE * msg, int32_t len)
{
	int ret = -1;
	/* if write returns -1, error */
	if (write(fd, &len, sizeof(int32_t)) > 0){
		ret = write(fd, msg, len);
	}
	return ret;
}


int BeaconRead (int fd, BYTE * msg, int32_t maxbuflen)
{
	int len = 0;
	int ret;
	/* blocking read waiting for a beacon */
	if (read(fd, &len, sizeof(int32_t)) > 0 ){
		if (len <= maxbuflen){
			ret = read(fd, msg, len);
			/* ensure the whole message has been readed */
			while (ret != len){
				ret += read(fd, msg+ret, len-ret);
			}
			return len;
		}else{
			return 0;
		}
	}else{
		/* End of socket */
		return -1;
	}
}



















