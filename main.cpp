#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
using json = nlohmann::json;

namespace lsp {
  class Message {
    public:
      std::string jsonrpc = "2.0";
  };

  template <typename T>
  class ResponseMessage : public Message {
    public:
      int id;
      T result;

    ResponseMessage(int id, T result) : id(id), result(result) {};
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResponseMessage, id, result, jsonrpc);
  };

  class InitializeResult {
    public:
      struct serverInfo {
        std::string name;
        std::string version;
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(serverInfo, name, version);
      } serverInfo;

      struct capabilities {
        struct completionProvider {
          bool resolveProvider = false;
          int textDocumentSync = 1;
          NLOHMANN_DEFINE_TYPE_INTRUSIVE(completionProvider, resolveProvider, textDocumentSync);
        } completionProvider;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(capabilities, completionProvider);
      } capabilities;

    InitializeResult(std::string name, std::string version): serverInfo({name, version}) {};
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InitializeResult, serverInfo, capabilities);
  };

  struct CompletionItem {
    std::string label;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompletionItem, label);
  };

  class CompletionList {
    public:
      bool isIncomplete;
      std::vector<CompletionItem> items;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompletionList, isIncomplete, items);
      CompletionList(bool isIncomplete, std::vector<CompletionItem> items): isIncomplete(isIncomplete), items(items) {};
  };

  CompletionList handle_completion(json request) {
    std::vector<CompletionItem> items = {
      (CompletionItem) { .label = "testtest" },
      (CompletionItem) { .label = "krokodil" },
      (CompletionItem) { .label = "pepe" }
    };

    return CompletionList(false, items);
  }

  std::optional<json> handle_request(json request) {
    if (request.contains("id")) {
      if (request.contains("method") && request["method"] == "initialize") {
        return ResponseMessage<InitializeResult>(request["id"], InitializeResult("my-custom-sfcc-lsp", "0.0.1"));
      } else if (request.contains("method") && request["method"] == "textDocument/completion") {
        return ResponseMessage<CompletionList>(request["id"], handle_completion(request));
      }
    }
    return {};
  }
}

int main(void) {
  setvbuf(stdin, NULL, _IONBF, 0);
  std::ofstream log_file("./lsp.log");

  log_file << "Starting!\n";
  while (true) {
    std::string content_length_line;
    std::getline(std::cin, content_length_line, '\r');
    // eating up the remaining not needed bytes
    std::cin.get(); std::cin.get(); std::cin.get();

    std::string content_length_str = content_length_line.substr(
        content_length_line.find(":") + 2, content_length_line.size());

    int content_length = std::atoi(content_length_str.c_str());
    char* request_str = (char*)malloc(content_length);
    std::cin.read(request_str, content_length);
    log_file << request_str << '\n';

    if (json::accept(request_str)) {
      auto response = lsp::handle_request(json::parse(request_str));
      if (response.has_value()) {
        std::cout << "Content-Length: " << response.value().dump().size() << "\r\n\r\n" << response.value().dump();
        log_file << response.value().dump() << '\n';
      }
    } else {
      log_file << "Invalid request json.\n";
    }

    log_file.flush();
    free(request_str);
  }

  log_file.close();
  return 0;
}
