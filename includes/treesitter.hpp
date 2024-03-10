#ifndef SFCC_TREESITTER_HPP_
#define SFCC_TREESITTER_HPP_

#include <string>
#include <tree_sitter/api.h>

extern "C" const TSLanguage* tree_sitter_javascript(void);

namespace lsp {
  struct RequireLineInfo {
    std::string cartridge_file_path;
  };

  class TreeSitter {
    private:
      TSParser* ts_parser;
      const TSLanguage* lang;

    public:
      TreeSitter() {
        this->ts_parser = ts_parser_new();
        this->lang = tree_sitter_javascript();
        ts_parser_set_language(this->ts_parser, this->lang);
      }

      ~TreeSitter() {
        ts_parser_delete(this->ts_parser);
      }

      RequireLineInfo parse_require_line(std::string require_line);
  };
}

#endif // SFCC_TREESITTER_HPP_
