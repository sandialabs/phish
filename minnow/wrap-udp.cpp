#include <phish.hpp>

#include <iostream>

#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::output(0);
  phish::check();

  try
  {
    if(argc != 2)
      throw std::runtime_error("wrap-udp syntax: wrap-udp <port>");

    const int socket = ::socket(PF_INET6, SOCK_DGRAM, 0);
    if(socket == -1)
      throw std::runtime_error(std::string("socket(): ") + ::strerror(errno));

    struct sockaddr_in6 address;
    ::memset(&address, 0, sizeof(address));
    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(::atoi(argv[1]));
    if(-1 == ::bind(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)))
      throw std::runtime_error(std::string("bind(): ") + ::strerror(errno));

    std::vector<char> buffer(1024 * 1024); // 1 megabyte, unless you're a disk vendor.
    while(true)
    {
      const int bytes = ::recv(socket, &buffer[0], buffer.size() - 1, 0);
      buffer[bytes] = 0;
      //std::cerr << bytes << std::endl;
      //std::cerr << std::string(&buffer[0], bytes) << std::endl;
      phish::pack(&buffer[0]);
      phish::send();
    }

    ::close(socket);
    phish::exit();
  }
  catch(std::exception& e)
  {
    phish::error(e.what());
  }
}

