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

  server.up();

  return 0;
}
