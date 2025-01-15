#include "http_server.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

static std::string vecToStr(std::vector<std::string> strs) {
  if (strs.size() == 0) {
    return "";
  }

  std::string str = strs[0];
  for (size_t i = 1; i != strs.size(); ++i) {
    str += ", " + strs[i];
  }

  return str;
}

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

void HttpServer::registerHandler(std::string path, HttpMethod httpMethod,
                                 RequestHandler handler) {
  if (path == "" || path == "/") {
    path = "/index.html";
  }

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

std::string HttpServer::getContentType(const RequestInfo &requestInfo) {
  std::string extension =
      std::filesystem::path(requestInfo.path).extension().string();
  if (extension == ".html") {
    return "text/html";
  } else if (extension == ".css") {
    return "text/css";
  } else if (extension == ".js") {
    return "text/javascript";
  }

  return "text/plain";
}

void HttpServer::setHeader(
    std::unordered_map<std::string, std::vector<std::string>> &headers,
    std::string k, std::string v) {
  headers[k].push_back(v);
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

  if (requestInfo.path == "" || requestInfo.path == "/") {
    requestInfo.path = "/index.html";
  }

  // headers
  while (std::getline(requestStream, line) && line != "\r") {
    std::istringstream lineStream(line);
    std::string headerName, headerValue;
    lineStream >> headerName;
    headerName.erase(headerName.find(":"), 1); // erase ':'

    while (lineStream >> headerValue) {
      auto pos = headerValue.find(","); // erase ','
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
  printRequestInfo(requestInfo);

  auto h = handlers.find(requestInfo.method + requestInfo.path);
  std::string response;
  if (h != handlers.end()) {
    response = generateResponse(h->second(requestInfo));
  } else {
    response = generateResponse(responseError(404, "Not found"));
  }

  send(clientSocket, response.c_str(), response.size(), 0);
}

std::string HttpServer::generateResponse(const ResponseInfo &responseInfo) {
  std::ostringstream response;
  response << responseInfo.proto << " " << responseInfo.statusCode << " "
           << responseInfo.statusMessage << "\r\n";
  for (auto const &[k, v] : responseInfo.headers) {
    response << k << ": " << vecToStr(v) << "\r\n";
  }
  response << "Content-Length: " << responseInfo.body.size() << "\r\n";
  response << "Connection: close\r\n\r\n";
  response << responseInfo.body;
  return response.str();
}

void HttpServer::printRequestInfo(const RequestInfo &requestInfo) {
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
}

HttpServer::ResponseInfo
HttpServer::staticHandler(const HttpServer::RequestInfo &requestInfo) {
  std::filesystem::path projectPath = std::filesystem::current_path();
  std::string filePath = projectPath.string() + "/static" + requestInfo.path;

  std::ifstream f(filePath.c_str());
  if (!f.is_open()) {
    std::cerr << "Can't open file: " << filePath.c_str() << std::endl;
    return ResponseInfo{.statusCode = 404,
                        .proto = requestInfo.proto,
                        .statusMessage = "Not Found",
                        .body = ""};
  }

  std::ostringstream response;
  response << f.rdbuf();

  std::unordered_map<std::string, std::vector<std::string>> headers;
  setHeader(headers, "Content-Type", getContentType(requestInfo));

  return ResponseInfo{.statusCode = 200,
                      .proto = requestInfo.proto,
                      .statusMessage = "OK",
                      .body = response.str(),
                      .headers = headers};
}

HttpServer::ResponseInfo HttpServer::responseError(int errorCode,
                                                   std::string errorMessage) {
  std::string body = "<h1>" + errorMessage + "</h1>";
  std::unordered_map<std::string, std::vector<std::string>> headers;
  setHeader(headers, "Content-Type", "text/html");
  return ResponseInfo{.statusCode = errorCode,
                      .proto = "HTTP/1.1",
                      .statusMessage = "OK",
                      .body = body,
                      .headers = headers};
}
