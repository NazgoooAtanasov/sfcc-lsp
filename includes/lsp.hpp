#ifndef SFCC_LSP_HPP_
#define SFCC_LSP_HPP_

#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace lsp {
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

  struct CompletionItem {
    std::string label;
    std::string insertText;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompletionItem, label, insertText);
  };

  struct Message {
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

  struct ServerInfo {
    std::string name;
    std::string version;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ServerInfo, name, version);
  };

  struct CompletionProvider {
      bool resolveProvider = false;
      NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompletionProvider, resolveProvider);
  };

  struct Capabilities {
    CompletionProvider completionProvider;
    int textDocumentSync = 1;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Capabilities, completionProvider, textDocumentSync);
  };

  class InitializeResult {
    public:
      ServerInfo serverInfo;
      Capabilities capabilities;

    InitializeResult(std::string name, std::string version): serverInfo({name, version}) {};
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(InitializeResult, serverInfo, capabilities);
  };

  class CompletionList {
    public:
      bool isIncomplete;
      std::vector<CompletionItem> items;

      CompletionList(bool isIncomplete, std::vector<CompletionItem> items): isIncomplete(isIncomplete), items(items) {};
      NLOHMANN_DEFINE_TYPE_INTRUSIVE(CompletionList, isIncomplete, items);
  };

  struct TextDocument {
      std::string uri;
      int version;
      NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocument, uri, version);
  };

  struct TextDocumentContentChangeEvent {
      std::string text;
      NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentContentChangeEvent, text);
  };

  struct DidChangeTextDocumentParams {
    TextDocument textDocument;
    std::vector<TextDocumentContentChangeEvent> contentChanges;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DidChangeTextDocumentParams, textDocument, contentChanges);
  };

  class LSP {
    private:
      std::vector<CompletionItem> items;
      std::map<std::string, std::string> documents;

    public:
      LSP(std::vector<CompletionItem> items) : items(items) {};
      LSP(std::vector<CompletionItem> items, std::map<std::string, std::string> documents) : items(items), documents(documents) {};

      CompletionList handle_completion(json request);
      std::optional<json> handle_request(json request);
      void handle_notification(json request);
  };
}

#endif // SFCC_LSP_HPP_
