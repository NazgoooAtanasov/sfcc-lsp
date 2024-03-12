#include "includes/lsp.hpp"
#include "workspace.hpp"
#include <filesystem>

using namespace lsp;

std::optional<std::string> LSP::get_document(std::string uri) {
  if (this->documents.contains(uri)) {
    return this->documents[uri];
  }
  return {};
}

std::string LSP::to_uri(std::string file_path) {
  std::string file_uri = "file://";
  file_uri.append(file_path);
  return file_uri;
}

CompletionList LSP::handle_completion(json request) {
  return CompletionList(false, this->items);
}

std::optional<std::vector<Location>> LSP::handle_definition(json request) {
  std::string textDocumentUri = request["params"]["textDocument"]["uri"];
  Position position = request["params"]["position"].template get<Position>();

  auto document = this->get_document(textDocumentUri);
  if (!document.has_value()) {
    return {};
  }

  std::vector<std::string> lines;
  std::string buff;
  std::stringstream ss(document.value());
  while (std::getline(ss, buff, '\n')) {
    lines.push_back(buff);
  }

  auto req = this->ts.parse_require_line(lines[position.line]);
  if (!req.has_value()) {
    return {};
  }

  std::string require = req.value().cartridge_file_path;
  require.insert(0, "/*");
  require.insert(0, this->current_path);
  require.append(".js");

  // @TODO: globbing is not an ideal solution. it is limitited on platform level
  // plus no recursive search. implement file traverser that should get cached.
  glob_t glob_result = {0};
  size_t glob_status = glob(require.c_str(), GLOB_TILDE, NULL, &glob_result);

  std::string error;
  if (glob_status != 0) {
    std::stringstream ss;
    ss << strerror(errno) << std::endl;
    error = ss.str();
    return {};
  }

  std::vector<Location> locations;
  for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
    Location location = {
      .uri = this->to_uri(glob_result.gl_pathv[i]),
      .range = (Range){
        .start = (Position) { .line = 0, .character = 0 },
        .end = (Position) { .line = 0, .character = 0, }
      }
    };

    locations.push_back(location);
  }
  globfree(&glob_result);

  if (locations.size() <= 0) {
    return {};
  }

  return locations;
}

std::optional<std::vector<CartridgeEntry>> LSP::handle_cartridges(json request) {
  std::vector<CartridgeEntry> cartridges_list;
  workspace::cartridges cartridges;
  workspace::traverse(this->current_path, cartridges);

  if (cartridges.empty()) {
    return {};
  }

  for (const auto& cartridge : cartridges) {
    cartridges_list.push_back((CartridgeEntry) {
        .file_path = cartridge.second,
        .file_name = cartridge.first,
        });
  }

  return cartridges_list;
}

std::optional<json> LSP::handle_request(json request) {
    if (request["method"] == "initialize") {
      return ResponseMessage<InitializeResult>(request["id"], InitializeResult("my-custom-sfcc-lsp", "0.0.1"));
    } 

    if (request["method"] == "textDocument/completion") {
      return ResponseMessage<CompletionList>(request["id"], this->handle_completion(request));
    }

    if (request["method"] == "textDocument/definition") {
      auto location = this->handle_definition(request);
      if (!location.has_value()) {
        return {};
      }

      return ResponseMessage<std::vector<Location>>(request["id"], location.value());
    }

    if (request["method"] == "sfcc-lsp/workspace/cartridges") {
      auto location = this->handle_cartridges(request);
      if (!location.has_value()) {
        return {};
      }
      return ResponseMessage<std::vector<CartridgeEntry>>(request["id"], location.value());
    }

    return {};
}

void LSP::handle_notification(json request) {
    if (request["method"] == "textDocument/didChange") {
      auto didChangeNotification = request.template get<NotificationMessage<DidChangeTextDocumentParams>>();
      for (auto& element : didChangeNotification.params.contentChanges) {
        this->documents.insert({didChangeNotification.params.textDocument.uri, element.text});
      }
    }

    if (request["method"] == "textDocument/didOpen") {
      auto didOpenNotification = request.template get<NotificationMessage<DidOpenTextDocumentParams>>();
      this->documents.insert({didOpenNotification.params.textDocument.uri, didOpenNotification.params.textDocument.text});
    }
}
