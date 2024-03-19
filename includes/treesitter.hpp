#ifndef SFCC_TREESITTER_HPP_
#define SFCC_TREESITTER_HPP_

#include <tree_sitter/api.h>
#include <optional>
#include <string>
#include <vector>
#include <sstream>

extern "C" const TSLanguage* tree_sitter_javascript(void);

namespace lsp {
  struct RequireLineInfo {
    std::string cartridge_file_path;
  };

  class TreeSitter {
    private:
      TSParser* ts_parser;
      const TSLanguage* lang;
      void parse_object_toks(TSNode n, std::vector<std::string>& container, std::string& line);
      std::string get_node_str_from_points(TSNode n, std::string& line);

    public:
      TreeSitter() {
        this->ts_parser = ts_parser_new();
        this->lang = tree_sitter_javascript();
        ts_parser_set_language(this->ts_parser, this->lang);
      }

      ~TreeSitter() {
        ts_parser_delete(this->ts_parser);
      }

      std::optional<RequireLineInfo> parse_require_line(std::string require_line);
      std::optional<std::vector<std::string>> parse_object_expansion(std::string line);
      std::optional<std::string> get_variable_decl(std::string file_content, std::string var_name);
  };
}

#endif // SFCC_TREESITTER_HPP_
