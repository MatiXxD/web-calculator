#include "http_server.hpp"

#include <iostream>
#include <sstream>

HttpServer::HttpServer(int port)
    : port{port}, serverSocket{-1}, isRunning{false}, serverAddress{} {}

HttpServer::~HttpServer() { down(); }

void HttpServer::up() {
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket == -1) {
    throw HttpServerError("Can't create socket");
  }

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  if (bind(serverSocket, (struct sockaddr *)&serverAddress,
           sizeof(serverAddress)) == -1) {
    close(serverSocket);
    throw HttpServerError("Bind failed");
  }

  if (listen(serverSocket, MAX_CONNECTIONS) == -1) {
    close(serverSocket);
    throw HttpServerError("Listen failed");
  }

  isRunning = true;
  std::cout << "Server listening on port " << port << std::endl;

  mainLoop();
}

void HttpServer::down() {
  if (isRunning) {
    isRunning = false;
    close(serverSocket);
    std::cout << "Server stopped";
  }
}

void HttpServer::registerHandler(const std::string &path, HttpMethod httpMethod,
                                 RequestHandler handler) {
  std::string method = "";
  if (httpMethod == HttpMethod::GET) {
    method = "GET";
  } else if (httpMethod == HttpMethod::POST) {
    method = "POST";
  } else if (httpMethod == HttpMethod::PUT) {
    method = "PUT";
  } else if (httpMethod == HttpMethod::DELETE) {
    method = "DELETE";
  }

  handlers[method + path] = handler;
}

void HttpServer::mainLoop() {
  while (isRunning) {
    sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);
    int clientSocket =
        accept(serverSocket, (struct sockaddr *)&clientAddress, &clientLen);
    if (clientSocket == -1) {
      if (isRunning) {
        std::cerr << "Accept failed" << std::endl;
      }
      continue;
    }

    handleClient(clientSocket);
    close(clientSocket);
  }
}

HttpServer::RequestInfo HttpServer::parseRequest(const std::string &request) {
  RequestInfo requestInfo = {};
  std::istringstream requestStream(request);
  std::string line;

  // request line
  if (std::getline(requestStream, line)) {
    std::istringstream lineStream(line);
    lineStream >> requestInfo.method >> requestInfo.path >> requestInfo.proto;
  }

  // headers
  while (std::getline(requestStream, line) && line != "\r") {
    std::istringstream lineStream(line);
    std::string headerName, headerValue;
    lineStream >> headerName;
    headerName.erase(headerName.find(":"), 1);  // erase ':'

    while (lineStream >> headerValue) {
      auto pos = headerValue.find(",");  // erase ','
      if (pos != std::string::npos) {
        headerValue.erase(pos, 1);
      }
      requestInfo.headers[headerName].push_back(headerValue);
    }
  }

  // body
  if (requestStream.good()) {
    std::getline(requestStream, requestInfo.body, '\0');
  }

  return requestInfo;
}

void HttpServer::handleClient(int clientSocket) {
  char buffer[MAX_MSG_SIZE];
  ssize_t bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
  if (bytesRead <= 0) {
    return;
  }

  buffer[bytesRead] = '\0';
  std::string request(buffer);
  RequestInfo requestInfo = parseRequest(request);

  // TESTING PART
  std::cout << "REQUEST:\n";
  std::cout << requestInfo.method << ' ' << requestInfo.path << ' '
            << requestInfo.proto << std::endl;

  std::cout << "\nHEADERS:\n";
  for (const auto &[k, v] : requestInfo.headers) {
    std::cout << k << ": ";
    for (const std::string &s : v) {
      std::cout << s << ' ';
    }
    std::cout << std::endl;
  }
  std::cout << "\nBODY:\n";
  std::cout << requestInfo.body << std::endl;
  std::cout << "---------------------------------------------\n\n";
  // TESTING PART

  // TO-DO: Handlers logic
  // std::string body;
  // std::unordered_map<std::string, std::string> headers;
  //
  // std::istringstream requestStream(request);
  // std::string method, path;
  // requestStream >> method >> path;
  //
  // auto h = handlers.find(method + path);
  // std::string response;
  // if (h != handlers.end()) {
  //   response = generateResponse(h->second(request));
  // }

  // TO-DO: replace request with response
  send(clientSocket, request.c_str(), request.size(), 0);
}
