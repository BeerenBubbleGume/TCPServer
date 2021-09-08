#include "TCPServer.h"
#include <chrono>
#include <pthread.h>

TCPServer::TCPServer(const uint16_t port, _handler_function handler) : port(port), handler(handler) {};

TCPServer::~TCPServer(){
    if(_status == status::up){
        stop();
    }
}

void TCPServer::setHandler(_handler_function handler){this->handler = handler;}

uint16_t TCPServer::getPort() const {return port;}
uint16_t TCPServer::setPort(const uint16_t port){
    this->port = port;
    restart();
    return port;
}

TCPServer::status TCPServer::restart(){
    if(_status == status::up){
        stop();
    }
    return start();
}

void TCPServer::joinLoop() {handler_thread.join();}

int TCPServer::Client::loadData() {return recv(socket, buffer, buffer_size, 0);}
char* TCPServer::Client::getData() {return buffer;}
bool TCPServer::Client::sendData(const char* buffer, const size_t size) const {
    if(send(socket, buffer, size, 0) < 0) {return false;}
    return true;
}

TCPServer::status TCPServer::start(){
    struct sockaddr_in server;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    server.sin_family = AF_INET;
    serv_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(serv_socket == -1) return _status = status::error_socket_init;
    if(bind(serv_socket, (struct sockaddr*)&server, sizeof(server)) < 0) return _status = status::error_socket_bind;
    if(listen(serv_socket, 3) < 0) return _status = status::error_socket_listening;
    return _status;
}

void TCPServer::stop() {
    _status = status::close;
    close(serv_socket);
    joinLoop();
    for(std::thread& cl_thr : client_handler_threads){
        cl_thr.join();
    }
    client_handler_threads.clear();
    client_handling_end.clear();
}

void TCPServer::handlingLoop(){
    while(_status == status::up){
        int client_socket;
        struct sockaddr_in client_addr;
        int addrlen = sizeof(struct sockaddr_in);
        if((client_socket = accept(serv_socket, (struct sockaddr*)&client_addr, (socklen_t*)&addrlen)) >= 0 && _status == status::up){
            client_handler_threads.push_back(std::thread([this, &client_socket, &client_addr]{
                handler(Client(client_socket, client_addr));
                client_handling_end.push_back(std::this_thread::get_id());
            }));
        }
        if(!client_handling_end.empty()){
            for(std::list<std::thread::id>::iterator id_it = client_handling_end.begin(); !client_handling_end.empty(); id_it = client_handling_end.begin()){
                for(std::list<std::thread>::iterator thr_it = client_handler_threads.begin(); thr_it == client_handler_threads.end(); ++thr_it){
                    if(thr_it->get_id() == *id_it){
                        thr_it->join();
                        client_handler_threads.erase(thr_it);
                        client_handling_end.erase(id_it);
                        break;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
}

TCPServer::Client::Client(int socket, struct sockaddr_in address) : socket(socket), address(address) {}
TCPServer::Client::~Client(){
    shutdown(socket, 0);
    close(socket);
}

uint32_t TCPServer::Client::getHost() { return address.sin_addr.s_addr;}
uint16_t TCPServer::Client::getPort() {return address.sin_port;}