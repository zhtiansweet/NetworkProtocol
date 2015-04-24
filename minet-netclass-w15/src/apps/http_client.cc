#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <string>

#define BUFSIZE 1024

using std::string;

int write_n_bytes(int fd, char * buf, int count);
void error(int sock, char *req);

int main(int argc, char * argv[]) {
    char * server_name = NULL;
    int server_port = 0;
    char * server_path = NULL;

    int sock = 0;
    int rc = -1;
    int datalen = 0;
    int connect=0;
    bool ok = true;
    struct sockaddr_in sa;
    FILE * wheretoprint = stdout;
    struct hostent * site = NULL;
    char * req = NULL;
    char * reqform = NULL;

    char buf[BUFSIZE + 1];
    char * bptr = NULL;
    char * bptr2 = NULL;
    char * endheaders = NULL;

    string response = "";
    string response_header = "";
    string status="";
   
    struct timeval timeout;
    fd_set set;

    /*parse args */
    if (argc != 5) {
	fprintf(stderr, "usage: http_client k|u server port path\n");
	exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];

    /* initialize minet */
    if (toupper(*(argv[1])) == 'K') { 
	minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') { 
	minet_init(MINET_USER);
    } else {
	fprintf(stderr, "First argument must be k or u\n");
	exit(-1);
    }

    /* create socket */
    sock=minet_socket(SOCK_STREAM);
    if (sock==-1){
        fprintf(stderr, "Failed to create socket\n");
        exit(-1);
    }

    // Do DNS lookup
    /* Hint: use gethostbyname() */
    site=gethostbyname(server_name);
    if (site==NULL){
        fprintf(stderr, "Fail to find the hostname\n");
        minet_close(sock);
        exit(-1);
    }

    /* set address */
    memset(&sa,0,sizeof(sa));
    sa.sin_port=htons(server_port);
    sa.sin_addr.s_addr=* (unsigned long *)site->h_addr_list[0];
    sa.sin_family=AF_INET;

    /* connect socket */ 
    connect=minet_connect(sock,(struct sockaddr_in *)&sa);
    if (connect!=0){
        fprintf(stderr, "Failed to connect socket\n");
        minet_close(sock);
        exit(-1);
    }

    /* send request */

    reqform="GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n";
     if(server_path[0] == '/')
        server_path = server_path + 1;
    req=(char *)malloc(strlen(server_name)+strlen(server_path)+26);
    sprintf(req, reqform, server_path, server_name);
    datalen=strlen(req)+1;

    rc=minet_write(sock, req, datalen);
    if (rc<0){
        fprintf(stderr, "Failed to send request\n");
        error(sock,req);
    }
    
    /* wait till socket can be read */   
    FD_ZERO(&set);
    FD_SET(sock, &set);
    if (FD_ISSET(sock, &set)==0){
        fprintf(stderr, "Failed to set the file descriptor\n");
        error(sock,req);
    }

    /* Hint: use select(), and ignore timeout for now. */
    rc=minet_select(sock+1, &set, 0, 0, &timeout);
    if (rc<0){
        fprintf(stderr, "Failed to select\n");
        error(sock,req);
    }

    /* first read loop -- read headers */
    memset(buf, 0, sizeof(buf));
    endheaders="\r\n\r\n"; 
    string::size_type pos;  
    while((rc=(minet_read(sock,buf,BUFSIZE)))>0){
        buf[rc] = '\0';
        response=response+(string)buf;
        if ((pos= response.find(endheaders,0)) != string::npos){
            response_header = response.substr(0,pos);
            response = response.substr(pos + 4);
            break;
        }
    }

    /* examine return code */   
    //Skip "HTTP/1.0" 
    status = response_header.substr(response_header.find(" ") + 1);
    status = status.substr(0, status.find(" "));

    // Normal reply has return code 200
    if (status!="200"){
        ok=false;
    }

    /* print first part of response */
    fprintf(wheretoprint, response_header.c_str());
    fprintf(wheretoprint, endheaders);
    fprintf(wheretoprint, response.c_str());

    /* second read loop -- print out the rest of the response */
    while ((rc = minet_read(sock, buf, BUFSIZE)) > 0) {
        buf[rc] = '\0';
      if(ok) {                                                                                        
        fprintf(wheretoprint, buf);                                                                           
      } else { 
        fprintf(stderr, buf);
      }
    } 
    
    /*close socket and deinitialize */
    minet_close(sock);
    minet_deinit();
    free(req);

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

int write_n_bytes(int fd, char * buf, int count) {
    int rc = 0;
    int totalwritten = 0;

    while ((rc = minet_write(fd, buf + totalwritten, count - totalwritten)) > 0) {
	totalwritten += rc;
    }
    
    if (rc < 0) {
	return -1;
    } else {
	return totalwritten;
    }
}

void error(int sock, char *req){
    minet_close(sock);
    free(req);
    exit(-1);
}
