#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <future>
#include <chrono>
#include <memory>
#include <vector>

#include <unistd.h>
#include <signal.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


// #include "getopt.h"
// #include "linenoise.h"

struct TcpServerConn{
  sockaddr_in sockaddr_conn;
  std::future<uint64_t> fut;
  std::future<uint64_t> fut_input;
  bool isRunning;

  TcpServerConn() = delete;
  TcpServerConn(const TcpServerConn&) =delete;
  TcpServerConn& operator=(const TcpServerConn&) =delete;
  TcpServerConn(int sockfd_conn, sockaddr_in sockaddr_conn_){
    sockaddr_conn = sockaddr_conn_;
    isRunning = true;
    fut = std::async(std::launch::async, &TcpServerConn::AsyncTcpConn, &isRunning, sockfd_conn);
  }
  ~TcpServerConn(){
    printf("TcpServerConn deconstructing\n");
    if(fut.valid()){
      isRunning = false;
      fut.get();
    }
    printf("TcpServerConn deconstruction done\n");
  }

  static uint64_t AsyncTcpConn(bool* isTcpConn, int sockfd){
    printf("AsyncTcpServerConn is started\n");
    uint64_t n_ev = 0;
    *isTcpConn = true;
    while (*isTcpConn){



      auto &ev_front = layer->Front();
      if(ev_front){
        auto ev = ev_front;
        layer->PopFront();
        std::string ev_raw = ev->m_raw;
        char *writeptr = ev_raw.empty()?nullptr:&ev_raw[0];
        size_t bytes_read = ev_raw.size();
        while (bytes_read > 0 && *isTcpConn) {
          int written = write(sockfd, writeptr, bytes_read);
          if (written < 0) {
            fprintf(stderr, "ERROR writing to the TCP socket. errno: %d\n", errno);
            if (errno == EPIPE) {
              *isTcpConn = false;
              break;
            }
          } else {
            bytes_read -= written;
            writeptr += written;
          }
        }
        n_ev++;
      }
      else{
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        continue;
      }
    }

    close(sockfd);
    *isTcpConn = false;
    printf("AsyncTcpServerConn is exited\n");
    return n_ev;
  }
};


struct TcpServer{
  std::future<uint64_t> fut;
  bool isRunning;

  TcpServer() = delete;
  TcpServer(const TcpServer&) =delete;
  TcpServer& operator=(const TcpServer&) =delete;

  TcpServer(altel::Layer* layer, short int port){
    isRunning = true;
    fut = std::async(std::launch::async, &TcpServer::AsyncTcpServer, &isRunning, layer, port);
  }
  ~TcpServer(){
    printf("TcpServer deconstructing\n");
    if(fut.valid()){
      isRunning = false;
      fut.get();
    }
    printf("TcpServer deconstruction done\n");
  }

  static uint64_t AsyncTcpServer(bool* isTcpServ, altel::Layer* layer, short int port){
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    // std::string now_str = TimeNowString("%y%m%d%H%M%S");
    printf("AsyncTcpServ is starting...\n");

    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0)
      fprintf(stderr, "ERROR opening socket");

    /*allow reuse the socket binding in case of restart after fail*/
    int itrue = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &itrue, sizeof(itrue));

    sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
      fprintf(stderr, "ERROR on binding. errno=%d\n", errno);
    listen(sockfd, 1);
    printf("AsyncTcpServ is listenning...\n");

    std::vector<std::unique_ptr<TcpServerConn>> tcpConns;

    *isTcpServ = true;
    while(*isTcpServ){
      sockaddr_in cli_addr;
      socklen_t clilen = sizeof(cli_addr);

      int sockfd_conn = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); //wait for the connection
      if (sockfd_conn < 0){
        if( errno == EAGAIN  || errno == EWOULDBLOCK){
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          continue;
        }
        fprintf(stderr, "ERROR on accept \n");
        throw;
      }

      tcpConns.push_back(std::make_unique<TcpServerConn>(layer, sockfd_conn, cli_addr));
      printf("new connection from %03d.%03d.%03d.%03d\n",
             (cli_addr.sin_addr.s_addr & 0xFF), (cli_addr.sin_addr.s_addr & 0xFF00) >> 8,
             (cli_addr.sin_addr.s_addr & 0xFF0000) >> 16, (cli_addr.sin_addr.s_addr & 0xFF000000) >> 24);

      // for(auto & conn: tcpConns){
      //   if(conn && conn->isRunning){
      //     conn.reset();
      //   }
      // }// TODO: erase
    }

    printf("AsyncTcpServ is removing connections...\n");
    tcpConns.clear();
    printf("AsyncTcpServ is exited\n");

    return 0;
  }
};



