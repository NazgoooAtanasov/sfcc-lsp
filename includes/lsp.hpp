#ifndef SFCC_LSP_HPP_
#define SFCC_LSP_HPP_

#include <cerrno>
#include <fstream>
#include <string>
#include <ranges>
#include <glob.h>
#include <nlohmann/json.hpp>
#include <treesitter.hpp>
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

  struct Location {
    std::string uri;
    Range range;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Location, uri, range);
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
    bool definitionProvider = true;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Capabilities, completionProvider, textDocumentSync, definitionProvider);
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

  struct TextDocumentItem {
    std::string uri;
    std::string languageId;
    int version;
    std::string text;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TextDocumentItem, uri, languageId, version, text);
  };

  struct DidOpenTextDocumentParams {
    TextDocumentItem textDocument;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DidOpenTextDocumentParams, textDocument);
  };

  struct DidChangeTextDocumentParams {
    TextDocument textDocument;
    std::vector<TextDocumentContentChangeEvent> contentChanges;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DidChangeTextDocumentParams, textDocument, contentChanges);
  };

  struct CartridgeEntry {
    std::string file_path;
    std::string file_name;
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CartridgeEntry, file_name, file_path);
  };

  typedef std::map<std::string, std::vector<std::string>> FileCache;

  class LSP {
    private:
      std::vector<CompletionItem> items;
      std::map<std::string, std::string> documents;
      std::optional<std::string> get_document(std::string uri);
      std::string current_path;
      TreeSitter ts;
      // @TODO: this should be onto a file so that the lsp does not traverse the files on every attach
      // this should start handling new files. for now, new files will not be included in the cache,
      // therefore they will not appear as possible locations
      FileCache fc;

      std::optional<std::vector<Location>> goto_definition_require_line(std::string line);

      CompletionList handle_completion(json request);
      std::optional<std::vector<Location>> handle_definition(json request);
      std::optional<std::vector<CartridgeEntry>> handle_cartridges(json request);

      std::string to_uri(std::string file_path);
      void prepare_log_file();
      void build_file_cache(void);

    public:
      std::ofstream log_file;

      LSP(std::vector<CompletionItem> items, std::string current_path);
      LSP(std::vector<CompletionItem> items, std::string current_path, std::map<std::string, std::string> documents);
      ~LSP() { this->log_file.close(); }

      std::optional<json> handle_request(json request);
      void handle_notification(json request);
  };
}

#endif // SFCC_LSP_HPP_
