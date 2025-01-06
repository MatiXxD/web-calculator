#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <functional>
#include <string>
#include <unordered_map>

class HttpServerError : public std::exception {
 public:
  HttpServerError(const std::string &msg) : msg(msg) {}
  const char *what() const noexcept override { return msg.c_str(); }

 private:
  std::string msg;
};

class HttpServer {
 public:
  static constexpr int MAX_CONNECTIONS = 20;
  static constexpr int MAX_MSG_SIZE = 30720;

  using RequestHandler = std::function<std::string(const std::string &req)>;
  enum class HttpMethod { GET, POST, PUT, DELETE };
  struct RequestInfo {
    std::string method;
    std::string path;
    std::string proto;
    std::string body;
    std::unordered_map<std::string, std::vector<std::string>> headers;
  };

  explicit HttpServer(int port);
  ~HttpServer();

  void up();
  void down();

  void registerHandler(const std::string &path, HttpMethod httpMethod,
                       RequestHandler handler);

 private:
  bool isRunning;
  int serverSocket;
  int port;
  sockaddr_in serverAddress;

  std::unordered_map<std::string, RequestHandler> handlers;

  void mainLoop();
  RequestInfo parseRequest(const std::string &request);
  void handleClient(int clientSocket);

  std::string generateResponse(const std::string &body,
                               const std::string &statusMessage = "OK",
                               int statusCode = 200);
};

#endif
