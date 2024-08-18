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

using namespace std;

const size_t k_max_msg = 4096;

static void msg( const char* msg);
static int32_t read_full( int connfd , char* buff , int n );
static int32_t write_all( int connfd , const char* buff , int n );

enum{
	STATE_REQ = 0;
	STATE_RES = 1;
	STATE_END = 2;
};



struct Conn{
	int fd = -1;
	uint32_t state = 0;//either STATE_REQ or STATE_RES
	
	//buffer for reading
	size_t rbuf_size = 0;
	uint8_t rbuf[4 + k_max_msg];
	
	//buffer for writing
	size_t wbuf_size = 0;
	size_t wbuf_sent = 0;
	uint8_t wbuf[4 + k_max_msg];

};



static int32_t one_request(int connfd)
{

	//initializing buffer
	char rbuf[4 + k_max_msg + 1 ];
	
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
	if( len > k_max_msg )
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


static void fd_set_nb( int fd )
{
	errno = 0;
	int flags = fctnl(fd,F_GETFL,0);
	if( errno ){
		die("fctnl error");
		return;
	}

	flags |= O_NONBLOCK;

	errno = 0;
	(void)fctnl(fd,F_SETFL,flags);
	if( errno )
	{
		die("fctnl error");
	}
}


struct pollfd {
    int fd;         /* File descriptor to monitor
                       Example: 
                       - fd = 0: Monitor standard input (stdin).
                       - fd = 4: Monitor a socket or file descriptor returned by open() or socket(). */
                       
    short events;   /* Events to monitor (input)
                       Example:
                       - POLLIN: Monitor for data to read.
                       - POLLOUT: Monitor if writing is possible without blocking.
                       - POLLERR: Monitor for error conditions.
                       You can combine these using bitwise OR, e.g., (POLLIN | POLLOUT). */
                       
    short revents;  /* Events that actually occurred (output)
                       Example: 
                       - POLLIN: Data is available to read.
                       - POLLHUP: The connection was closed by the other side.
                       - POLLERR: An error has occurred.
                       After poll(), check this field to see what happened. */
};

static void connection_io(Conn* conn)
{
    /*likely a case in which server is receiving and processing a client requests*/
    if( conn->state == STATE_REQ)
    {
        state_req(conn);
    }

static void conn_put( vector<Conn*> &fd2conn, struct Conn *conn)
{
    if( fd2conn.size() <= (size_t)conn->fd ){
        fd2conn.resize(conn->fd + 1);
    }

    fd2conn[conn->fd] = conn;
}

static void state_req(Conn *conn)
{
    while(try_fill_buffer(conn)){}
}

static bool try_fill_buffer(Conn* conn)
{
    //try to fill the buffer
    assert(conn->rbuf_size < sizeof( conn->rbuf ));
    ssize_t rv = 0;
    do{
        size_t cap = sizeof( conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd , &conn->rbuf[conn->rbuf],  cap);
    }while( rv < 0 && errno == EINTR);
    //if there is an error in reading and system call is interrupted ,read again 
    /* rv > 0 , indicates , successfully read "rv"  bytes from file descriptor into the buffer*/
    
    //neagative value indicates an error occured during the read opertion
   //RESOURCE TEMPORARY UNAVAILABLE : EAGAIN
    if( rv < 0 && errno = EAGAIN){
        //got EAGAIN, stop
        return false;
    }

    if( rv < 0 )
    {
        msg("read () ERRRO");
        conn->state = STATE_END;
        return false;
    }

    if( rv == 0 )
    {
        if( conn->rbuf_size > 0 )
        {
            msg("unexpected EOF");
        }
        else 
        {
            msg("EOF");
   

        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size() <= sizeof(conn->rbuf);

    //Try to process the requests one by one
    while( try_one_request(conn){};
    return ( conn->state == state->REQ);
}

static bool try_flush_buffer(Conn *conn)
{

    ssize_t rv = 0;
    do
    {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(connf->fd , &conn->wbuf[conn->wbuf_sent] , remain);
    }
    while( rv < 0 && errno == EINTR);
   
    if( rv < 0 && errno = EAGAIN) return false;

    if( rv < 0 ) 
    {
        msg("write() error");
        conn->state = STATE_END;    
        return false;
    }


    conn->wbuf_sent += (size_t)rv;
    assert(conn->wbuf_sent <= conn->wbuf_size);

    if( conn->wbuf_sent == conn->wbuf_size )
    {
        //response was fully sent , change state back
        conn->state = STATE_REQ;
        conn->wbuf_sent = 0;
        conn->wbuf_size = 0;
        return false;
    }

    //still got some data in wbuf , could try to write again
    return true;

}

static void state_res(Conn *conn)
{
    while( try_flush_buffer(conn)){};
    
}
static bool try_one_request(Conn *conn)
{
    //try to parse a request from the buffer
    if( conn->buff_size < 4 ) 
    {
        printf("no enough buffer size");
        return false;
    }

    unit32_t msg_len = 0;
    memcpy(&len,&conn->rbuf[0],4);
    if( len > k_max_msg)
    {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }

    if( 4 + len > conn->rbuf_size )
    {
        printf("not enough data in buffer , will retry in next iteration");
        return false;
    }

    printf("client says : %*.s\n",len,&conn->rbuf[4]);


    //generating echoing response
    memcpy(&conn->wbuf[0],&len,4);
    memcpy(&wbuf[4],&conn->rbuf[4],len);
    conn->wbuf_size = 4 + len;

    //remove the request from the buffe
    //the feqeuwn memove is inefficent
    //need better handling for production code
    size_t remain = conn->rbuf_size - 4 + len ;
    if( remain){
        memmove(conn-rbuf,&conn->rbuf[4+len],remain);
    }
    connf->rbuf_size = remain;

    //change state
    conn->state = STATE_RES;
    state_res(conn);

    //continue the outer requst if the requst was fullt processed
    return (conn->state == STATE_REQ);   
}

   


static int32_t accept_new_conn(vector<Conn*> fd2conn,int fd)
{    //accept
    struct sockaddr_in client_addr{};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept( fd,(struct sockaddr *)&client_addr,&socklen);
    if( connfd < 0 ) 
    {
        msg("accept() error ");
        return -1;//error
    }

    //set the new connection fd to nonblocking mode
    fd_set_nb(connfd);
    
    //create the stuct Conn
    struct Conn *conn = (struct Conn*)malloc(sizeof(struct Conn));
    if( !conn )
    {
        close(connfd);
        return -1;
    }

    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;

    conn_put(fd2conn,conn);
    return 0'
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
    };

	//a map of all client connections , keyed by fd
	vector<Conn*> fd2conn;

	// set the listen fd to non-blocking mode
	fd_set_nb(fd);

	//the event loop
	vector<struct pollfd> poll_args;
	while( true )
	{
		//preapre the args to poll
		poll_args.clear();

		//for convenience , the listening fd is put in the first postion	
		struct poll_pfd = {fd,POLLIN,0};//create an temp poll_fd -> pfd
		poll_args.push_back(pfd);
	    /* listeing fd 'fd' listens for new connectiosn and if they are there adds it to fd2conn*/



		//connection fds
		for( Conn *conn : fd2conn )
		{
			if( !conn ) continue;
			struct pollfd pfd ={};
			pfd.fd = conn->fd;
			pfd.events = ( conn->state == STATE_REQ ) ? POLLIN : POLLOUT;
			pfd.events = pfd.events | POLLERR;
			poll_args.push_back(pfd);		
		}

        // poll for active fds to see if any of them are ready for I/O operations
		int rv = poll( poll_args.data() , (nfds_t)poll_args.size() , 1000 );
		if( rv < 0 ) 
		{
			die("poll");
		}
		/* poll_args.data() returns pointer to the first poll_args strucutre element of poll_args */ 
		

		//process active connections
        for( size_t i = 1; i < poll_args.size() ; ++i )
        {
            if( poll_args[i].revents )
            {
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);

                if( Conn->state == STATE_END )
                {
                    //client closed normally, or somehting bad happened
                    //destroy this connection
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        if( poll_args[0].revents )
        {
            (void)accept_new_conn(fd2conn,fd);
        }



    close(fd);
    return 0;
}



