#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
#include <tr1/unordered_map>

#include <signal.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>

int nframe = 0;
bool first = true;
int perframe = 50;

int anomaly = 0;
int correct = 0;
int pfalse = 0;
int nfalse = 0;

// thresh = N,M
// flag as anomaly if key appears N times with value=1 <= M times
// thresh = 12,2 flags many false positives

int nthresh = 24;
int mthresh = 4;

std::tr1::unordered_map<uint64_t, std::pair<int,int> > kv;

int minframe,maxframe;

void stats(int dummy=0)
{
  printf("\n");
  printf("%d frames sent\n",maxframe-minframe+1);
  printf("%d frames received\n",nframe);
  printf("%g percentage received\n",100.0*nframe / (maxframe-minframe+1));
  printf("%d datums received\n",nframe*perframe);
  printf("%d unique keys\n",kv.size());

  std::tr1::unordered_map<uint64_t, std::pair<int,int> >::iterator i;
  int kmax = 0;
  for (i = kv.begin(); i != kv.end(); i++) {
    std::pair<int,int>& tuple = i->second;
    if (tuple.first > kmax) kmax = tuple.first;
  }

  printf("%d max occurrence of any key\n",kmax);
  printf("%d anomalies detected\n",anomaly);
  printf("%d true anomalies\n",correct);
  printf("%d false positives\n",pfalse);
  printf("%d false negatives\n",nfalse);

  exit(0);
}

int main(int argc, char* argv[])
{
  signal(SIGINT,stats);

  try {
    if (argc != 2) throw std::runtime_error("syntax: anomaly <port>");
    const int port = atoi(argv[1]);

    const int socket = ::socket(PF_INET6, SOCK_DGRAM, 0);
    if (socket == -1)
      throw std::runtime_error(std::string("socket(): ") + ::strerror(errno));

    int socket_buffer_size = 1024 * 1024;
    socklen_t socket_buffer_size_size = sizeof(socket_buffer_size);
    ::setsockopt(socket, SOL_SOCKET, SO_RCVBUF, 
                 &socket_buffer_size, socket_buffer_size_size);

    struct sockaddr_in6 address;
    ::memset(&address, 0, sizeof(address));
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(::atoi(argv[1]));
    if (-1 == ::bind(socket, reinterpret_cast<sockaddr*>(&address),
                    sizeof(address)))
      throw std::runtime_error(std::string("bind(): ") + ::strerror(errno));

    int maxbuf = 64*perframe;
    std::vector<char> buffer(maxbuf);

    while (true) {
      const int bytes = ::recv(socket, &buffer[0], buffer.size() - 1, 0);
      if (bytes == 0) break;
      buffer[bytes] = '\0';

      if (first) {
        first = false;
        sscanf(&buffer[0],"frame %d",&minframe);
      }
      sscanf(&buffer[0],"frame %d",&maxframe);

      char *p = strtok(&buffer[0],"\n");
      for (int i = 0; i < perframe; i++) {
        int64_t key = atol(strtok(NULL,",\n"));
        int64_t value = atoi(strtok(NULL,",\n"));
        int64_t truth = atoi(strtok(NULL,",\n"));

        // store datum in hash table
        // value = (N,M)
        // N = # of times key has been seen
        // M = # of times flag = 1
        // M < 0 means already seen N times

        if (!kv.count(key)) kv[key] = std::pair<int,int>(1,value);
        else {
          std::pair<int,int>& tuple = kv[key];
          if (tuple.second < 0) {
            ++tuple.first;
            continue;
          }

          ++tuple.first;
          tuple.second += value;
          if (tuple.first >= nthresh) {
            if (tuple.second > mthresh) {
              if (truth) {
                nfalse++;
                printf("FALSE NEG %ld %d\n",key,truth);
              }
            } else {
              anomaly++;
              if (truth) {
                correct++;
                printf("ANOMALY %ld %d\n",key,truth);
              } else {
                pfalse++;
                printf("FALSE POS %ld %d\n",key,truth);
              }
            }
            tuple.second = -1;
          }
        }
      }
      
      ++nframe;
    }
    
    stats(0);
    return 0;
    
  } catch(std::exception& e){
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
