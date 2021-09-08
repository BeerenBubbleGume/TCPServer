#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <cstdint>
#include <functional>
#include <thread>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static constexpr uint16_t buffer_size = 8096;

struct TCPServer {
    class Client;

    typedef std::function<void(Client)> _handler_function;

    enum class status : uint8_t {
        up = 0,
        error_socket_init = 1,
        error_socket_bind = 2,
        error_socket_listening = 3,
        close = 4
    };

private:
    uint16_t port;
    status _status = status::close;
    _handler_function handler;

    std::thread handler_thread;
    std::list<std::thread> client_handler_threads;
    std::list<std::thread::id> client_handling_end;
    int serv_socket;
    void handlingLoop();

public:
    TCPServer(const uint16_t port, _handler_function handler);
    ~TCPServer();
    void setHandler(_handler_function handler);
    uint16_t getPort() const;
    uint16_t setPort(const uint16_t port);

    status getStatus() const {return _status;}

    status restart();
    status start();

    void stop();

    void joinLoop();
};

class TCPServer::Client{
    int socket;
    struct sockaddr_in address;
    char buffer[buffer_size];
public:
    Client(int socket, struct sockaddr_in addres);
    ~Client();

    uint32_t getHost();
    uint16_t getPort();

    int loadData();
    char* getData();
    

    bool sendData(const char* buffer, const size_t size) const;
};

#endif