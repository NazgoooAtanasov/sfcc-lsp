#include "includes/lsp.hpp"
#include "workspace.hpp"
#include <cstdlib>

using namespace lsp;

std::vector<std::string> split_string(const std::string& input, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0, end = 0;

    while ((end = input.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(input.substr(start, end - start));
        start = end + 1;
    }

    tokens.push_back(input.substr(start));

    return tokens;
}

bool is_forbidden(std::string& path_str) {
    return path_str.find(".git") != std::string::npos ||
      path_str.find("node_modules") != std::string::npos ||
      path_str.find("test") != std::string::npos;
}

void process_dir(std::filesystem::path path, lsp::FileCache& fc) {
  auto path_str = path.string();

  if (is_forbidden(path_str)) return; 

  for (const auto& entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_directory()) {
      process_dir(entry.path(), fc);
    } else {
      auto file_path = entry.path().string();

      // @TODO: this `/` delim is not cross platform
      std::vector<std::string> toks = split_string(file_path, '/');

      size_t cartridge_pos = 0;
      for (size_t i = 0; i < toks.size(); ++i) {
        if (toks.at(i) == "cartridge") {
          cartridge_pos = i;
        }
      }

      if (cartridge_pos == 0) {
        continue;
      }

      std::string cartridge_file = std::accumulate(
          toks.begin() + cartridge_pos,
          toks.end(),
          std::string(),
          [](std::string a, std::string b) {
            return a + "/" + b;
          });

      if (fc.find(cartridge_file) == fc.end()) {
        fc.insert({cartridge_file, std::vector<std::string> { file_path }});
        continue;
      }

      fc[cartridge_file].push_back(file_path);
    }
  }
}

void LSP::prepare_log_file() {
  const char* home = std::getenv("HOME");
  assert(home != NULL && "This has been ran on a non posix system");
  std::filesystem::path log_dir = std::filesystem::path(home) / ".sfcclsp";
  std::filesystem::path log_path = log_dir / "lsp.log";
  std::filesystem::create_directories(log_dir);
  this->log_file = std::ofstream(log_path);
  this->build_file_cache();
}

lsp::LSP::LSP(std::vector<CompletionItem> items, std::string current_path) :
  items(items),
  current_path(current_path) 
{
  prepare_log_file();
};

lsp::LSP::LSP(std::vector<CompletionItem> items, std::string current_path, std::map<std::string, std::string> documents) : 
  items(items),
  current_path(current_path),
  documents(documents) 
{
  prepare_log_file();
};

void LSP::build_file_cache(void) {
  process_dir(std::filesystem::path(this->current_path), this->fc);
}

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


std::optional<std::vector<Location>> LSP::goto_definition_require_line(std::string line) {
  auto req = this->ts.parse_require_line(line);

  if (!req.has_value())  {
    return {};
  }

  // */cartridge/something/something
  std::string require = req.value().cartridge_file_path;
  require.append(".js"); // */cartridge/something/something.js
  require.replace(0, 1, ""); // /cartridge/something/something.js

  if (this->fc.find(require) == this->fc.end()) {
    return {};
  }

  std::vector<Location> locations;
  for (const auto& path : this->fc[require]) {
    locations.push_back(
      (Location) {
        .uri = this->to_uri(path),
        .range = (Range) {
          .start = (Position) {.line = 0, .character = 0},
          .end   = (Position) {.line = 0, .character = 0},
        }
      });
  }

  return locations;
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

  auto require_line = this->goto_definition_require_line(lines[position.line]);
  if (require_line.has_value()) {
    return require_line.value();
  }

  auto object_tokens = this->ts.parse_object_expansion(lines[position.line]);
  if (object_tokens.has_value() && object_tokens.value().size() > 0) {
    auto module = object_tokens.value().at(0);
    auto variable_decl_line = this->ts.get_variable_decl(document.value(), module);
    if (!variable_decl_line.has_value()) {
      return {};
    }

    this->log_file << "The line for the definition is '" << variable_decl_line.value() << std::endl;

    auto require_line = this->goto_definition_require_line(variable_decl_line.value());
    if (require_line.has_value()) {
      return require_line.value(); 
    }

  }

  return {};
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
