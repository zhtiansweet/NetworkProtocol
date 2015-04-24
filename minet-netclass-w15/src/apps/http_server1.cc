#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string>
using std::string;

#define BUFSIZE 1024
#define FILENAMESIZE 100

int handle_connection(int);
int writenbytes(int,char *,int);
int readnbytes(int,char *,int);

int main(int argc,char *argv[])
{
  int server_port;
  int sock,sock2;
  struct sockaddr_in sa,sa2;
  int rc;

  /* parse command line args */
  if (argc != 3)
  {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }
  server_port = atoi(argv[2]);
  if (server_port < 1500)
  {
    fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
    exit(-1);
  }

  /* initialize and make socket */
  if (toupper(*(argv[1])) == 'K')
  {
    minet_init(MINET_KERNEL);
  } 
  else if (toupper(*(argv[1])) == 'U')
  {
    minet_init(MINET_USER);
  } 
  else 
  {
    fprintf(stderr, "First argument must be k or u\n");
    exit(-1);
  }

  sock = minet_socket(SOCK_STREAM);//sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == -1)
  {
     minet_perror("Failed to create minet socket!\n");
     exit(-1);
  }
  /* set server address*/
  memset(&sa,0,sizeof(sa));
  sa.sin_port=htons(server_port);//sa is the accept socket
  sa.sin_addr.s_addr=htonl(INADDR_ANY);
  sa.sin_family=AF_INET;
  /* bind listening socket */
  rc = minet_bind(sock, &sa);
  if (rc < 0)
  {
     minet_perror("Failed to bind listening socket!\n");
     exit(-1);
  }
  /* start listening */
  rc = minet_listen(sock,5);
  if (rc < 0)
  {
     minet_perror("Failed to start listening!\n");
     exit(-1);
  }
  /* connection handling loop */
  while(1)
  {
    /* handle connections */
    sock2 = minet_accept(sock,&sa2);
    if (sock2 < 0)
    {
      minet_perror("Failed to accept connection!\n");
      continue;//try again
    }
    rc = handle_connection(sock2);
  }
}


int handle_connection(int sock2)
{
  //char filename[FILENAMESIZE+1];
  int rc;
  int fd;
  struct stat filestat;
  char buf[BUFSIZE+1];
  //char *headers;
  char *endheaders;
  //char *bptr;
  int datalen=0;
  char *ok_response_f = "HTTP/1.0 200 OK\r\n"\
                      "Content-type: text/plain\r\n"\
                      "Content-length: %d \r\n\r\n";
  char ok_response[100];
  char *notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"\
                         "Content-type: text/html\r\n\r\n"\
                         "<html><body bgColor=black text=white>\n"\
                         "<h2>404 FILE NOT FOUND</h2>\n"
                         "</body></html>\n";
  bool ok=true;

  memset(buf, 0, sizeof(buf));
  endheaders = "\r\n\r\n";
  string headers="";
  string request="";
  string::size_type pos;
  /* first read loop -- get request and headers*/
  while ((rc = minet_read(sock2, buf, BUFSIZE))>0){
    buf[rc]='\0';
    request = request + (string)buf;
    if ((pos = request.find(endheaders, 0)) != string::npos){
      headers = request.substr(0,pos);
      request = request.substr(pos+4);
      break;
    }
  }

  if(rc<0){
    perror("fail to read the socket");
    close(sock2);
    exit(-1);
  }

  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/
  string filename = headers.substr(headers.find(" ") + 1);
  filename = filename.substr(0, filename.find(" "));

  if (filename[0] == '/'){
    filename = filename.substr(1);
  }
  
  char filename1[FILENAMESIZE];
  strcpy(filename1, filename.c_str()); //convert string to char array

  if (filename1 == NULL){
    ok = false;
  }  

  else{
    if (strlen(filename1)>FILENAMESIZE){
      fprintf(stderr, "file name is too long");
      minet_close(sock2);
      exit(-1);
    }
  }

    /* try opening the file */
  fd = open(filename1, O_RDONLY);
  if (fd<0){
    ok = false;
  }
  /* send response */
  if (ok)
  {

    rc = stat(filename1, &filestat);
    if (rc<0){
      perror("fail to get file status");
      minet_close(sock2);
      exit(-1);
    }
    /* send headers */
    sprintf(ok_response,ok_response_f,filestat.st_size);
    rc = writenbytes(sock2, ok_response, strlen(ok_response));
    if (rc<0){
      perror("fail to write bytes to socket");
      minet_close(sock2);
      exit(-1);
    }

    /* send file */
    while ((rc = minet_read(fd, buf, BUFSIZE))!=0){
        if (rc<0)
        {
            perror("fail to read the file");
            minet_close(sock2);
            exit(-1);
        }
      rc = writenbytes(sock2, buf, rc);
      datalen=datalen+rc;
      if (datalen == filestat.st_size){
        break;
      }
    }

    if (rc<0){
      perror("fail to read the file");
      minet_close(sock2);
      exit(-1);
    }
  }

  else // send error response
  {
    rc = writenbytes(sock2, notok_response, strlen(notok_response));
    if (rc<0){
      perror("error in write bytes to file");
      minet_close(sock2);
      exit(-1);
    }
  }

  /* close socket and free space */
  close(fd);
  minet_close(sock2);
  if (ok)
    return 0;
  else
    return -1;
}

int readnbytes(int fd,char *buf,int size)
{
  int rc = 0;
  int totalread = 0;
  while ((rc = minet_read(fd,buf+totalread,size-totalread)) > 0)
    totalread += rc;

  if (rc < 0)
  {
    return -1;
  }
  else
    return totalread;
}

int writenbytes(int fd,char *str,int size)
{
  int rc = 0;
  int totalwritten =0;
  while ((rc = minet_write(fd,str+totalwritten,size-totalwritten)) > 0)
    totalwritten += rc;

  if (rc < 0)
    return -1;
  else
    return totalwritten;
}

