#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include <cstdlib>
using json = nlohmann::json;

static std::map<std::string, std::string> m;

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
    friend void to_json(nlohmann ::json &nlohmann_json_j,
                        const ResponseMessage &nlohmann_json_t) {
      nlohmann_json_j["id"] = nlohmann_json_t.id;
      nlohmann_json_j["result"] = nlohmann_json_t.result;
      nlohmann_json_j["jsonrpc"] = nlohmann_json_t.jsonrpc;
    }
    friend void from_json(const nlohmann ::json &nlohmann_json_j,
                          ResponseMessage &nlohmann_json_t) {
      nlohmann_json_j.at("id").get_to(nlohmann_json_t.id);
      nlohmann_json_j.at("result").get_to(nlohmann_json_t.result);
      nlohmann_json_j.at("jsonrpc").get_to(nlohmann_json_t.jsonrpc);
    };
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
      std::optional<int> rangeLength;
      Range range;

      NLOHMANN_DEFINE_TYPE_INTRUSIVE(textDocumentContentChangeEvent, text, range);
    } contentChanges;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DidChangeTextDocumentParams, textDocument, contentChanges);
  };

  CompletionList handle_completion(json request) {
    std::vector<CompletionItem> items = {
      (CompletionItem) { .label = "testtest" },
      (CompletionItem) { .label = "krokodil" },
      (CompletionItem) { .label = "pepe" } };

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

  void handle_notification(json request) {
    if (request["method"] == "textDocument/didChange") {
      auto didChangeNotification = request.template get<lsp::DidChangeTextDocumentParams>();
    }
  }
}

int main(void) {
  setvbuf(stdin, NULL, _IONBF, 0);
  std::ofstream log_file("./lsp.log");

  log_file << "Starting lsp!!!\n";

  while (true) {
    std::string content_length_line;
    std::getline(std::cin, content_length_line, '\r'); std::cin.get();
    // eating up the remaining not needed bytes
    std::cin.get(); std::cin.get();

    std::string content_length_str = content_length_line.substr(
        content_length_line.find(":") + 2, content_length_line.size());

    int content_length = std::atoi(content_length_str.c_str());

    char* request_str = (char*)malloc((content_length + 1) * sizeof(char));
    memset(request_str, '\0', sizeof(char) * content_length);

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
      lsp::handle_notification(request);
    }


    log_file.flush();
    memset(request_str, '\0', sizeof(char) * content_length);
    free(request_str);
  }

  log_file.close();
  return 0;
}
