#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

void init();
void *thread(void *vargp);
char *parse_uri(char* uri,char* hostname,char* path,char *port);
int to_server(char *hostname,char *path,char *headers,char *port);
int URI_hit(char *uri);
int LRU();
void cache_reader(int index,int fd);
void cache_writer(int index,char *uri,char *headers,char *body,int blen);
/* You won't lose style points for including this long line in your code */
static const char *proxy_cnt_hdr = "Proxy-Connection: close\r\n";
static const char *connect_hdr = "Connection: close\r\n";
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

/* global variables */
int readcnt;
sem_t mutex,w;
#define cache_cnt 10

struct cache{
    char cache_uri[MAXLINE];              /* save uri */
    char cache_hdrs[MAXLINE];             /* save response headers */
    char cache_body[MAX_OBJECT_SIZE];     /* save response body */
    int  body_len;                        /* the length of body */
    int  visit_time;                      /* relate to LRU */
};

struct cache c[cache_cnt];

int main(int argc, char **argv)
{
    Signal(SIGPIPE, SIG_IGN);
    int listenfd,*connfdp;
    char hostname[MAXLINE],port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    
    /*check command-line args */
    if(argc != 2){
        fprintf(stderr,"usage: %s <port>\n",argv[0]);
        exit(1);
    }

    init();
    listenfd = Open_listenfd(argv[1]);
    
    while(1){
        clientlen = sizeof(clientaddr);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd,(SA *)&clientaddr,&clientlen);
        Getnameinfo((SA *) &clientaddr,clientlen,hostname,MAXLINE,
                port,MAXLINE,0);
        printf("Accept connection from (%s %s)\n",hostname,port);
        Pthread_create(&tid,NULL,thread,connfdp);
    }
    Pthread_exit(NULL);
}

/* initial cache and global variables */
void init(){
    int i;
    readcnt = 0;
    Sem_init(&mutex,0,1);
    Sem_init(&w,0,1);
    for(i = 0;i < cache_cnt;i++){
        c[i].cache_uri[0] = '\0';
        c[i].cache_hdrs[0] = '\0';
        c[i].cache_body[0] = '\0';
        c[i].body_len = 0;
        c[i].visit_time = 0;
    }
}
void *thread(void *vargp){
    int proxy_fd,index,cont_len;
    char port[MAXLINE];
    char *illegal = "";
    char buf[MAXLINE],method[MAXLINE],uri[MAXLINE],version[MAXLINE];
    char headers[MAXLINE],res_hdrs[MAXLINE],body[MAX_OBJECT_SIZE];
    char con_len[MAXLINE];
    char hostname[MAXLINE],path[MAXLINE];
    rio_t rio;
 
    int fd = *((int *)vargp);
    Pthread_detach(pthread_self());
    Free(vargp);   
 
    Rio_readinitb(&rio,fd);
    Rio_readlineb(&rio,buf,MAXLINE);
    printf("Request headers:\n");
    printf("%s",buf);
    sscanf(buf,"%s %s %s",method,uri,version);
    
    /* if method != GET */
    if(strcasecmp(method,"GET")){
        printf("sorry,our poor proxy only support GET!\n");
        return NULL;
    }
    
    /* find cached or not */
    if((index = URI_hit(uri)) < cache_cnt){
        /* if cached */
        cache_reader(index,fd);
        close(fd);
        return NULL;
    } 
    
    /* read and save headers */
    Rio_readlineb(&rio, buf, MAXLINE);
    strcat(headers,buf);
    while(strcmp(buf, "\r\n")) {          
	Rio_readlineb(&rio, buf, MAXLINE);
        strcat(headers,buf);
    }

    /* check whether uri is legal and fill host path and port */
    if(parse_uri(uri,hostname,path,port) == illegal){
        printf("illegal uri\n");
        return NULL;
    }
    printf("here?\n");
    /* connection failed */
    if((proxy_fd = to_server(hostname,path,headers,port)) == -1){
        printf("connection failed!\n");
        return NULL;
    }

    /* proxy receive content from server and forward it to client */
    /* response headers */ 
    Rio_readinitb(&rio,proxy_fd);
    Rio_readlineb(&rio,buf,MAXLINE);
    strcat(res_hdrs,buf);
    Rio_writen(fd,buf,strlen(buf));
    while(strcmp(buf,"\r\n")){
        Rio_readlineb(&rio,buf,MAXLINE);
        strcat(res_hdrs,buf);
        Rio_writen(fd,buf,strlen(buf));
        if(strstr(buf,"Content-Length")){
            sscanf(buf,"Content-Length: %s",con_len);
            cont_len = atoi(con_len);
        }
        if(strstr(buf,"Content-length")){
            sscanf(buf,"Content-length: %s",con_len);
            cont_len = atoi(con_len);
        }
    }
   
    /* response body */
    if(cont_len > 0){
        int t = Rio_readnb(&rio,body,cont_len);
        Rio_writen(fd,body,t);
    }
    /* if it can be cached,cached it and use LRU */ 
    if(strlen(res_hdrs) + cont_len < MAX_OBJECT_SIZE){
        index = LRU();
        cache_writer(index,uri,res_hdrs,body,cont_len);
    }
    Close(proxy_fd);
    Close(fd);
    return NULL;
}

char *parse_uri(char *uri,char *hostname,char *path,char *port){
    char temp[MAXLINE];
    strcpy(temp,uri);
    char *illegal = "";
    char *host_start = strstr(temp,"//"); //find http:// 
    /* not found */
    if(host_start == NULL)   return illegal;
    else host_start += 2;
    /* read hostname port and path */
    char *port_start = strchr(host_start,':');  
    if(port_start != NULL) {
        char *path_start = strchr(port_start,'/');
        /* hostname port and path legal and exist */
        if(path_start != NULL){  
            sscanf(host_start,"%[^:]",hostname);
            sscanf(port_start + 1,"%[^/]",port);
            sscanf(path_start,"%s",path);
        }
        else  return illegal;
    }
    else {  
        char *path_start = strchr(host_start,'/');
        /* hostname and path legal and exist*/  
        if(path_start != NULL) {    
            sscanf(host_start,"%[^/]",hostname);
            sscanf(path_start,"%s",path);  
        }
        else   return illegal;
        /* default port */ 
        port[0] = '8';
        port[1] = '0';
        port[2] = '\0';
    }  
    /*default path */  
    if(strlen(path) == 1)  
        strcpy(path,"/index.html");  
    return uri;  
}

int to_server(char *hostname,char *path,char *headers,char *port){
    char buf[MAXLINE];
    char temp[MAXLINE];
    int proxy_fd;
    int has_headers = 0;
    /* open failed */
    if((proxy_fd = Open_clientfd(hostname,port)) == -1)
        return -1;
    strcpy(temp,headers);
    char *heads = temp;
    char *tail;
    
    /* send request and headers to server */
    sprintf(buf,"GET %s HTTP/1.0\r\n",path);
    Rio_writen(proxy_fd,buf,strlen(buf));
    while(strcmp(buf,"\r\n")){
        tail = strstr(heads,"\r\n");
        if(tail != NULL && strlen(tail)!= strlen("\r\n")){
            has_headers = 1;
            /* read headers */
            sscanf(heads,"%[^\r\n]",buf);
            /* use default Host */
            if(strstr(buf,"Host")){
                sprintf(buf,"Host: %s\r\n",hostname);
                Rio_writen(proxy_fd,buf,strlen(buf));
            }
            else if(strstr(buf,"User-Agent")) 
                Rio_writen(proxy_fd,(void *)user_agent_hdr,strlen(user_agent_hdr));
            else if(strstr(buf,"Proxy-Connection")) 
                Rio_writen(proxy_fd,(void *)proxy_cnt_hdr,strlen(proxy_cnt_hdr));
            else if(strstr(buf,"Connection")) {
                Rio_writen(proxy_fd,(void *)connect_hdr,strlen(connect_hdr));
            }
            /* forward other headers */
            else{
                strcat(buf,"\r\n");
                Rio_writen(proxy_fd,buf,strlen(buf));
            }
        }
        /* no more headers */
        else break;
        heads = tail + 2;
    }
    /*no header */
    if(!has_headers){
        sprintf(buf,"Host: %s\r\n",hostname);
        Rio_writen(proxy_fd,buf,strlen(buf));
        Rio_writen(proxy_fd,(void *)user_agent_hdr,strlen(user_agent_hdr));
        Rio_writen(proxy_fd,(void *)connect_hdr,strlen(connect_hdr));
        Rio_writen(proxy_fd,(void *)proxy_cnt_hdr,strlen(proxy_cnt_hdr));
    }
    /*headers end */
    Rio_writen(proxy_fd,"\r\n",strlen("\r\n"));
    return proxy_fd;
}
/* if find cached uri,return the index */
int URI_hit(char *uri){
    int i;
    P(&mutex);
    for(i = 0;i < cache_cnt;i++){
        if(!strcmp(uri,c[i].cache_uri)){
            V(&mutex);
            return i;
        }
    }
    V(&mutex);
    return i;    
}

/* find the max visit time and return its index */
int LRU(){
    int max_visit,max_index,i; 
    P(&mutex);
    max_visit = c[0].visit_time;
    max_index = 0;
    for(i = 0;i < cache_cnt;i++){
        if(c[i].visit_time > max_visit){
            max_visit = c[i].visit_time;
            max_index = i;
        }
    }
    V(&mutex);
    return max_index;
}

/* just like the reader and writer on the book 
* cache_reader send cached information to client
* cache_writer overwrite the cache
* use reader and header in order to deal with concurrency
*/
void cache_reader(int index,int fd){
        P(&mutex);
        readcnt++;
        if(readcnt == 1)
            P(&w);
        V(&mutex);
        P(&mutex);
        Rio_writen(fd,c[index].cache_hdrs,strlen(c[index].cache_hdrs));
        Rio_writen(fd,c[index].cache_body,c[index].body_len);
        c[index].visit_time = 0;
        V(&mutex);
        P(&mutex);
        readcnt--;
        if(readcnt == 0)
            V(&w);
        V(&mutex);
}

void cache_writer(int index,char *uri,char *headers,char *body,int blen){
    P(&w);
    int i;
    /* cache information in cache */
    strcpy(c[index].cache_uri,uri);
    strcpy(c[index].cache_hdrs,headers);
    strcpy(c[index].cache_body,body);
    c[index].body_len = blen;
    /* update visit time */
    for(i = 0;i < cache_cnt;i++)
        c[i].visit_time++;
    c[index].visit_time = 0;
    V(&w);
}
