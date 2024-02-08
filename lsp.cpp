#include "includes/lsp.hpp"

lsp::CompletionList lsp::LSP::handle_completion(json request) {
  return CompletionList(false, this->items);
}

std::optional<json> lsp::LSP::handle_request(json request) {
    if (request["method"] == "initialize") {
      return ResponseMessage<InitializeResult>(request["id"], InitializeResult("my-custom-sfcc-lsp", "0.0.1"));
    } 

    if (request["method"] == "textDocument/completion") {
      return ResponseMessage<CompletionList>(request["id"], handle_completion(request));
    }

    return {};
}

void lsp::LSP::handle_notification(json request) {
    if (request["method"] == "textDocument/didChange") {
      auto didChangeNotification = request.template get<lsp::NotificationMessage<lsp::DidChangeTextDocumentParams>>();
      for (auto& element : didChangeNotification.params.contentChanges) {
        this->documents.insert({didChangeNotification.params.textDocument.uri, element.text});
      }
    }
}
