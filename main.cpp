#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
#include <cstdlib>
#include <ranges>

#include "items.hpp"
#include "includes/lsp.hpp"

using json = nlohmann::json;

int main(void) {
  setvbuf(stdin, NULL, _IONBF, 0);
  std::ofstream log_file("./lsp.log");
  auto current_path = std::filesystem::current_path();
  auto current_path_str = current_path.string();
  current_path_str.push_back('/');
  log_file << "Starting lsp in " << current_path << std::endl;

  std::vector<lsp::CompletionItem> items = COMPLETION_REQUIRE_ITEMS;
  lsp::LSP lsp(items, current_path_str);

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
      auto response = lsp.handle_request(request);
      if (response.has_value()) {
        std::cout << "Content-Length: " << response.value().dump().size() << "\r\n\r\n" << response.value().dump();
        log_file << "[RESPONSE]: " << response.value().dump() << std::endl;
      }
    } else {
      lsp.handle_notification(request);
    }

    memset(request_str, '\0', sizeof(char) * content_length);
    free(request_str);
  }

  log_file.close();
  return 0;
}
