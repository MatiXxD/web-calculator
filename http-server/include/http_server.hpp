#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

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

  enum class HttpMethod { GET, POST, PUT, DELETE };

  struct RequestInfo {
    std::string method;
    std::string path;
    std::string proto;
    std::string body;
    std::unordered_map<std::string, std::vector<std::string>> headers;
  };

  struct ResponseInfo {
    int statusCode;
    std::string proto;
    std::string statusMessage;
    std::string body;
    std::unordered_map<std::string, std::vector<std::string>> headers;
  };

  using RequestHandler =
      std::function<ResponseInfo(const RequestInfo &requestInfo)>;

  explicit HttpServer(int port);
  ~HttpServer();

  void up();
  void down();

  void registerHandler(std::string path, HttpMethod httpMethod,
                       RequestHandler handler);

  static std::string getContentType(const RequestInfo &requestInfo);
  static void
  setHeader(std::unordered_map<std::string, std::vector<std::string>> &headers,
            std::string k, std::string v);

  static ResponseInfo staticHandler(const RequestInfo &requestInfo);
  static ResponseInfo responseError(int errorCode, std::string errorMessage);

private:
  bool isRunning;
  int serverSocket;
  int port;
  sockaddr_in serverAddress;

  std::unordered_map<std::string, RequestHandler> handlers;

  void mainLoop();
  RequestInfo parseRequest(const std::string &request);
  void handleClient(int clientSocket);

  std::string generateResponse(const ResponseInfo &responseInfo);

  void printRequestInfo(const RequestInfo &requestInfo);
};

#endif
