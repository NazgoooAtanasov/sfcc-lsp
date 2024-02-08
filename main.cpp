#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include <cstdlib>
#include <ranges>

#include "items.hpp"

using json = nlohmann::json;

namespace lsp {
  struct CompletionItem {
    std::string label;
    std::string insertText;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompletionItem, label, insertText);
  };
}

static std::map<std::string, std::string> m;
static std::vector<lsp::CompletionItem> items;

namespace lsp {
  class Message {
    public:
      std::string jsonrpc = "2.0";
  };

  template <typename T>
  class NotificationMessage : public Message {
    public:
      std::string method;
      T params;
      NLOHMANN_DEFINE_TYPE_INTRUSIVE(NotificationMessage, method, params);
  };

  template <typename T>
  class ResponseMessage : public Message {
    public:
      int id;
      T result;

    ResponseMessage(int id, T result) : id(id), result(result) {};
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ResponseMessage, id, result);
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
          NLOHMANN_DEFINE_TYPE_INTRUSIVE(completionProvider, resolveProvider);
        } completionProvider;

        int textDocumentSync = 1;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(capabilities, completionProvider, textDocumentSync);
      } capabilities;

    InitializeResult(std::string name, std::string version): serverInfo({name, version}) {};
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InitializeResult, serverInfo, capabilities);
  };

  class CompletionList {
    public:
      bool isIncomplete;
      std::vector<CompletionItem> items;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompletionList, isIncomplete, items);
      CompletionList(bool isIncomplete, std::vector<CompletionItem> items): isIncomplete(isIncomplete), items(items) {};
  };

  struct Position {
    int line;
    int character;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Position, line, character);
  };

  struct Range {
    Position start;
    Position end;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Range, start, end);
  };

  struct DidChangeTextDocumentParams {
    struct textDocument {
      std::string uri;
      int version;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE(textDocument, uri, version);
    } textDocument;

    struct textDocumentContentChangeEvent {
      std::string text;
      NLOHMANN_DEFINE_TYPE_INTRUSIVE(textDocumentContentChangeEvent, text);
    };
    std::vector<textDocumentContentChangeEvent> contentChanges;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DidChangeTextDocumentParams, textDocument, contentChanges);
  };

  CompletionList handle_completion(json request) {
    return CompletionList(false, items);
  }

  std::optional<json> handle_request(json request) {
    if (request["method"] == "initialize") {
      return ResponseMessage<InitializeResult>(request["id"], InitializeResult("my-custom-sfcc-lsp", "0.0.1"));
    } 

    if (request["method"] == "textDocument/completion") {
      return ResponseMessage<CompletionList>(request["id"], handle_completion(request));
    }

    return {};
  }

  void handle_notification(json request, std::ofstream* log_file) {
    if (request["method"] == "textDocument/didChange") {
      auto didChangeNotification = request.template get<lsp::NotificationMessage<lsp::DidChangeTextDocumentParams>>();
      for (auto& element : didChangeNotification.params.contentChanges) {
        m.insert({didChangeNotification.params.textDocument.uri, element.text});
      }
    }
  }
}

int main(void) {
  setvbuf(stdin, NULL, _IONBF, 0);
  std::ofstream log_file("./lsp.log");
  log_file << "Starting lsp!!!\n";

  items = COMPLETION_REQUIRE_ITEMS;
  while (true) {
    std::string content_length_line;
    std::getline(std::cin, content_length_line, '\r'); std::cin.get();
    // eating up the remaining not needed bytes
    std::cin.get(); std::cin.get();

    std::string content_length_str = content_length_line.substr(
        content_length_line.find(":") + 2, content_length_line.size());

    int content_length = std::atoi(content_length_str.c_str());

    char* request_str = (char*)malloc((content_length + 1) * sizeof(char));
    memset(request_str, '\0', sizeof(char) * content_length + 1);

    std::cin.read(request_str, content_length);
    log_file << "[REQUEST]: " << request_str << std::endl;

    if (!json::accept(request_str)) {
      log_file << "[ERROR]: Invalid request json." << std::endl;
      continue;
    }

    json request = json::parse(request_str);
    if (request.contains("id")) {
      auto response = lsp::handle_request(request);
      if (response.has_value()) {
        std::cout << "Content-Length: " << response.value().dump().size() << "\r\n\r\n" << response.value().dump();
        log_file << "[RESPONSE]: " << response.value().dump() << std::endl;
      }
    } else {
      lsp::handle_notification(request, &log_file);
    }

    memset(request_str, '\0', sizeof(char) * content_length);
    free(request_str);
  }

  log_file.close();
  return 0;
}
