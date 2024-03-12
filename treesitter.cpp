#include "treesitter.hpp"

std::optional<lsp::RequireLineInfo> lsp::TreeSitter::parse_require_line(std::string require_line)  {
  RequireLineInfo req_info;

  TSTree* tree = ts_parser_parse_string(
      this->ts_parser,
      nullptr,
      require_line.c_str(),
      require_line.size());

  TSNode root = ts_tree_root_node(tree);
  std::string query_str = "(variable_declarator name: (identifier) @required_vname value: (call_expression function: (identifier) arguments: (arguments (string(string_fragment) @cartridge_fpath))))";

  uint32_t err_offset;
  TSQueryError err_type;
  TSQuery* query = ts_query_new(
      tree_sitter_javascript(),
      query_str.c_str(),
      query_str.size(),
      &err_offset,
      &err_type);

  if (err_type != TSQueryErrorNone) {
    // @TODO: figure out how to report an error from line parsing. does it matter? 
    return {};
  }

  TSQueryCursor* cursor = ts_query_cursor_new();
  TSQueryMatch match = {0};
  ts_query_cursor_exec(cursor, query, root);

  size_t match_count = 0 ;
  while (ts_query_cursor_next_match(cursor, &match)) {
    match_count++;
    if (match.capture_count > 0) {
      for (size_t i = 0; i < match.capture_count; ++i) {
        uint32_t len;
        std::string capture_name = ts_query_capture_name_for_id(query, i, &len);

        if (capture_name == "cartridge_fpath") {
          TSPoint start_p = ts_node_start_point(match.captures[i].node);
          TSPoint end_p = ts_node_end_point(match.captures[i].node);

          req_info.cartridge_file_path = require_line.substr(start_p.column, end_p.column - start_p.column);
          break;
        }
      }
    }
  }

  if (match_count == 0) {
    return {};
  }

  return req_info;
}
