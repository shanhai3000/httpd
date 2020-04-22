#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int get_line(int sock, char *buf, int size){
    int i = 0;char c = '\0'; int n;
    while((i < size - 1) && (c != '\n')){
        n = recv(sock, &c, 1, 0);
        if(n > 0){
            if(c == '\r'){
                n = recv(sock, &c, 1, MSG_PEEK);
                if((n > 0) && (c == '\n'))
                    recv(sock, &c, 1, 0);
                else
                    c = '\n';
            }
            buf[i] = c;++i;
        }else{
            c = '\n';
        }
    }
    if(i > 0 && buf[i-1] != '\n'){
        buf[i] = '\n';
    }
    return i;
}
int start_up(u_short& port){
    struct sockaddr_in server_addr{};
    int httpd(0), on(1);
    if(!(httpd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))){
        perror("socket function error");exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //设定超时选项
    int re_use_addr_opt(0), rcv_timeout_op(0);struct timeval rcv_timeout = {6, 0};
    re_use_addr_opt = setsockopt(httpd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    rcv_timeout_op = setsockopt(httpd, SOL_SOCKET, SO_RCVTIMEO, (char *)& rcv_timeout, sizeof(struct timeval));
    if (re_use_addr_opt < 0 || rcv_timeout_op < 0){
        perror("set socket option error");exit(1);
    }
    if(bind(httpd, (struct sockaddr *)& server_addr, sizeof(server_addr)) < 0){
        perror("server bind socket error");exit(1);
    }

    if(port == 0){//使用0，会由内核分配端口号
        socklen_t server_socket_len = sizeof(server_addr);
        if(getsockname(httpd, (struct sockaddr *)& server_addr, &server_socket_len) == -1){
            perror("get socketname error");exit(1);
        }else{
            port = ntohs(server_addr.sin_port);
        }
    }
    if(listen(httpd, 5) < 0){
        perror("server listen error");exit(1);
    }

    return httpd;
}
void sig_child(int signo){
    pid_t pid;int stat;
    while((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
}
int main() {
    int server_listen_fd(-1);
    u_short port = 8999;
    server_listen_fd = start_up(port);
    int connect_fd(-1);
    struct sockaddr_in client_addr{};
    socklen_t client_socklen = sizeof(client_addr);
    int child_pid;
    signal(SIGCHLD, sig_child);
    char buf[1024];int request_chars_num(0);
    for(;;){
        if((connect_fd = accept(server_listen_fd,
                (struct sockaddr *)& client_addr,
                        &client_socklen)) < 0){
            if(connect_fd == EINTR){
                continue;
            }else{
                perror("accept error");exit(0);
            }
        }
        child_pid = fork();
        if(child_pid == 0){
            close(server_listen_fd);
            request_chars_num = get_line(connect_fd, buf, sizeof(buf));
            if(request_chars_num > 1){
                send(connect_fd, buf, sizeof(buf), 0);//echo
            }
            close(connect_fd);
        }
        close(connect_fd);
    }


    return 0;
}
