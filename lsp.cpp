#include "includes/lsp.hpp"

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

std::optional<Location> LSP::handle_definition(json request) {
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

  auto current_line = lines[position.line];

  size_t index = current_line.find("require");
  if (index == std::string::npos) {
    return {};
  }

  index = current_line.find("'");
  if (index == std::string::npos) {
    return {};
  }

  size_t end = current_line.find_last_of("'");
  if (end == std::string::npos) {
    return {};
  }

  auto require = current_line.substr(index + 1, (end - 1) - index);
  require.insert(0, 1, '*');
  require.insert(0, this->current_path);
  require.append(".js");

  glob_t glob_result = {0};
  size_t glob_status = glob(require.c_str(), GLOB_TILDE, NULL, &glob_result);
  std::string error;
  if (glob_status != 0) {
    std::stringstream ss;
    ss << strerror(errno) << std::endl;
    error = ss.str();
  }

  std::vector<std::string> filenames;
  for (size_t i = 0; i < glob_result.gl_pathc; ++i) {
    filenames.push_back(glob_result.gl_pathv[i]);
  }
  globfree(&glob_result);

  if (filenames.size() <= 0) {
    return {};
  }

  Location loc = {
    .uri = this->to_uri(filenames.at(0)),
    .range = (Range){
      .start = (Position) {
        .line = 0,
        .character = 0
      },
      .end = (Position) {
        .line = 0,
        .character = 0,
      }
    }
  };
  return loc;
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

      return ResponseMessage<Location>(request["id"], location.value());
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
