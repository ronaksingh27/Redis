#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <cassert>


const size_t k_msg_max = 4096;

static void msg( const char* msg);
static int32_t read_full( int connfd , char* buff , int n );
static int32_t write_all( int connfd , const char* buff , int n );

static int32_t one_request(int connfd)
{

	//initializing buffer
	char rbuf[4 + k_msg_max + 1 ];
	
	//read the message header
	errno = 0;
	int32_t err = read_full(connfd,rbuf,4);
	if( err )
	{
		if( errno == 0 ) 
		{
			msg("EOF");
		}
		else 
		{
			msg("read() error" );
		}
		
		return err;
	}

	//extract and validate the message length
	uint32_t len = 0;
	memcpy(&len,rbuf,4);//extracts the lenght of message
	if( len > k_msg_max )
	{
		msg("too long");
		return -1;
	}

	//read the message body
	err = read_full(connfd,&rbuf[4],len);
	if( err )
	{
		msg("read() error");
		return err;
	}

	//process the message
	rbuf[4+len] = '\0';
	printf("client says %s\n",&rbuf[4]);

	//send a reply to the client
	const char reply[] = "world";
	char wbuf[4 + sizeof(reply)];
	len = (uint32_t)strlen(reply);

	memcpy(wbuf,&len,4);//length of message
	memcpy(&wbuf[4],reply,len);

	return write_all(connfd,wbuf,4 + len);



}
static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

static void do_something(int connfd) {
    char rbuf[64] = {};
    ssize_t n = read(connfd, rbuf, sizeof(rbuf) - 1);
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("client says: %s\n", rbuf);

    char wbuf[] = "world";
    write(connfd, wbuf, strlen(wbuf));
}

static int32_t read_full( int connfd , char* buff , int n )
{
	while( n > 0 )
	{
		/*rv indicated the number of bytes successfully read from file descriptor*/ 
		ssize_t rv = read(connfd,buff,n);
		if( rv <= 0 ) 
		{
			return -1;
		}

		assert((size_t)rv <= n );
		n -= (size_t)rv;
		buff += rv;
	}
	return 0;
}


static int32_t write_all( int connfd , const char* buff , int n )
{
	while( n > 0 ) 
	{
		ssize_t rv = write(connfd,buff,n);
		if( rv <= 0 ) 
		{
			return -1;
		}
		
		assert((size_t)rv <= n );
		n -= (size_t)rv;
		buff += rv;
	}
	return 0;
}




int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    // this is needed for most server applications
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // bind
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(0);    // wildcard address 0.0.0.0
    int rv = bind(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("bind()");
    }

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {
        die("listen()");
    }

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t socklen = sizeof(client_addr);
        int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
        if (connfd < 0) {
            continue;   // error
        }

        //do_something(connfd);
	
	//only serves one clint connection at once
	while( true )
	{
		
		int32_t err = one_request(connfd);
		if( err)
		{
			break;
		}
		

	}
        close(connfd);
    }
    close(fd);
    return 0;
}



