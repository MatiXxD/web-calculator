#include <csignal>
#include <iostream>

#include "http_server.hpp"

HttpServer *httpServerPtr = nullptr;

void signalHandler(int signal) {
  if (signal == SIGINT && httpServerPtr) {
    httpServerPtr->down();
    exit(0);
  }
}

int main() {
  std::cout << "Test HttpServer" << std::endl;

  HttpServer server(8080);
  httpServerPtr = &server;
  std::signal(SIGINT, signalHandler);

  server.registerHandler("/", HttpServer::HttpMethod::GET,
                         HttpServer::staticHandler);
  server.registerHandler("/index.css", HttpServer::HttpMethod::GET,
                         HttpServer::staticHandler);
  server.registerHandler("/index.js", HttpServer::HttpMethod::GET,
                         HttpServer::staticHandler);

  server.up();

  return 0;
}
