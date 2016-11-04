#if 0
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

static int InitClientSocket(const char * ip, const char * port, BeaconMessageHandler * bmh, MessagePurpose trx)
{
    int sockfd, portno, n;
    int serverlen;
    int set_option_on = 1;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        exit(0);
    }

    /* gethostbyname: get the server's DNS entry */
    portno = atoi(port);
    server = gethostbyname(ip);
    if (server == NULL)
    {
        fprintf(stderr,"ERROR, no such host as %s\n", ip);
        exit(0);
    }
	serverlen = sizeof(serveraddr);
    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
	
	/* if we have to send beacons, we have to receive beacons, we create the socket and take a bind on it */
	if (trx == beacon_receiver)
	{
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &set_option_on, sizeof(set_option_on)) != 0)
		{
			fprintf(stderr, "set sockopt failed [%s]\n", strerror(errno));
			exit(0);		
		}

		if (bind(sockfd, (struct sockaddr *)&serveraddr, serverlen) != 0)
		{
			fprintf(stderr, "bind failed [%s]\n", strerror(errno));
			exit(0);
		}
	}

    bmh->fd = sockfd;
    bmh->addr = serveraddr;
    bmh->len = serverlen;

    return sockfd;
}
#if 0
static int InitClientSocket(const char * ip, const char * port)
{
	int fd;
	struct sockaddr_un addr;

	if ( (fd = socket(AF_INES, SOCK_DGRAM, 0)) == -1) {
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
#endif
int BeaconConnect (const char * ip, const char * port, BeaconMessageHandler * bmh, MessagePurpose trx)
{
	return InitClientSocket(ip, port, bmh, trx);
}

void BeaconClose (BeaconMessageHandler * bmh)
{
	close (bmh->fd);		
}

/* Just to ensure is a 32 bits int -> int32_t */
int BeaconWrite (BeaconMessageHandler * bmh, BYTE * msg, int32_t len, MsgSource m)
{
	int ret = -1;
	BYTE buffer[len + 1];
	/* if write returns -1, error */
	int32_t send_len = len + 1;
	buffer[0] = (BYTE) m;
	memcpy(buffer+1, msg, len);
	if (sendto(bmh->fd, &send_len, sizeof(int32_t), 0, (struct sockaddr *) &bmh->addr, bmh->len) > 0)
	{
		ret = sendto(bmh->fd, buffer, send_len, 0, (struct sockaddr *) &bmh->addr, bmh->len);
	}
	return ret;
}

/* */
int BeaconRead (BeaconMessageHandler * bmh, BYTE * msg, int32_t maxbuflen, MsgSource * m)
{
	BYTE buffer[maxbuflen];
	int len = 0;
	int ret = 0;
	/* blocking read waiting for a beacon */
	if (recvfrom(bmh->fd, &len, sizeof(int32_t), 0, (struct sockaddr *) &bmh->addr, &bmh->len) > 0 )
	{
		ret = recvfrom(bmh->fd, buffer, len, 0, (struct sockaddr *) &bmh->addr, &bmh->len);
		*m = (MsgSource) buffer[0];
		memcpy(msg, buffer+1, len - 1);
	}
	if (len == ret)
		return ret - 1;
	else
		return 0;
	#if 0
	if (read(bmh->fd, &len, sizeof(int32_t)) > 0 ){
		if (len <= maxbuflen){
			ret = read(bmh->fd, msg, len);
			/* ensure the whole message has been readed */
			while (ret != len){
				ret += read(bmh->fd, msg+ret, len-ret);
			}
			return len;
		}else{
			return 0;
		}
	}else{
		/* End of socket */
		return -1;
	}
	#endif
}
#endif
