#include "treesitter.hpp"

std::string lsp::TreeSitter::get_node_str_from_points(TSNode n, std::string &line) {
  TSPoint start = ts_node_start_point(n); 
  TSPoint end = ts_node_end_point(n);
  return line.substr(start.column, end.column - start.column);
}

void lsp::TreeSitter::parse_object_toks(TSNode n, std::vector<std::string>& container, std::string& line) {
  std::string obj = "object";
  TSNode obj_n = ts_node_child_by_field_name(n, obj.c_str(), obj.size());
  if (std::string(ts_node_type(obj_n)) == "identifier") {
    container.push_back(get_node_str_from_points(obj_n, line));
  }

  std::string prop = "property";
  TSNode prop_n = ts_node_child_by_field_name(n, prop.c_str(), prop.size());
  container.push_back(get_node_str_from_points(prop_n, line));

  TSNode parent = ts_node_parent(n);
  if (parent.id == nullptr) {
    return;
  }
  if (std::string(ts_node_type(parent)) == "member_expression") {
    parse_object_toks(parent, container, line);
  }
}

std::optional<lsp::RequireLineInfo> lsp::TreeSitter::parse_require_line(std::string require_line)  {
  ts_parser_reset(this->ts_parser);
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


std::optional<std::vector<std::string>> lsp::TreeSitter::parse_object_expansion(std::string line) {
  ts_parser_reset(this->ts_parser);

  TSTree* tree = ts_parser_parse_string(this->ts_parser, nullptr, line.c_str(), line.size());
  TSNode root_node = ts_tree_root_node(tree);

  std::string query_str = "(member_expression object: (identifier) property: (property_identifier)) @member_expr";
  uint32_t err_offs;
  TSQueryError err;
  TSQuery* query = ts_query_new(tree_sitter_javascript(), query_str.c_str(), query_str.size(), &err_offs, &err);
  if (err != TSQueryErrorNone) {
    // @TODO: figure out how to report an error from line parsing. does it matter? 
    return {};
  }

  TSQueryCursor* curs = ts_query_cursor_new();
  ts_query_cursor_exec(curs, query, root_node);
  TSQueryMatch m;

  uint32_t cap_idx;
  if (!ts_query_cursor_next_capture(curs, &m, &cap_idx)) {
    return {};
  }

  std::vector<std::string> tokens;
  parse_object_toks(m.captures[cap_idx].node, tokens, line);
  return tokens;
}

std::optional<std::string> lsp::TreeSitter::get_variable_decl(std::string file_content, std::string var_name) {
  std::vector<std::string> lines;
  std::string buff;
  std::stringstream ss(file_content);
  while (std::getline(ss, buff, '\n')) {
    lines.push_back(buff);
  }

  TSTree* tree = ts_parser_parse_string(this->ts_parser, nullptr, file_content.c_str(), file_content.size());
  TSNode root_node = ts_tree_root_node(tree);

  std::string query_str = "(_ [ (variable_declaration (_ name: (identifier) @module_name)) @decl (lexical_declaration (_ name: (identifier) @module_name)) @decl ])";
  uint32_t err_offs;
  TSQueryError err;
  TSQuery* query = ts_query_new(this->lang, query_str.c_str(), query_str.size(), &err_offs, &err);
  if (err != TSQueryErrorNone) {
    // @TODO: figure out how to report an error from line parsing. does it matter? 
    return {};
  }

  TSQueryCursor* curs = ts_query_cursor_new();
  ts_query_cursor_exec(curs, query, root_node);
  TSQueryMatch m;

  std::optional<TSNode> lex_decl = {};
  while (ts_query_cursor_next_match(curs, &m)) {
    if (m.capture_count < 0) {
      break;
    }

    uint32_t len;
    // lex_decl capture
    auto idx = m.captures[0].index;
    std::string first_capture = ts_query_capture_name_for_id(query, idx, &len);

    // module_name capture
    idx = m.captures[1].index;
    std::string second_capture = ts_query_capture_name_for_id(query, idx, &len);

    TSPoint start = ts_node_start_point(m.captures[1].node);
    TSPoint end = ts_node_end_point(m.captures[1].node);

    if (lines[start.row].substr(start.column, end.column - start.column) == var_name) {
      lex_decl = m.captures[0].node;
      break;
    }
  }

  if (lex_decl.has_value()) {
    TSPoint start = ts_node_start_point(lex_decl.value());
    TSPoint end = ts_node_end_point(lex_decl.value());
    return lines[start.row].substr(start.column, end.column - start.column);
  }

  return {};
}
