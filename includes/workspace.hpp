#ifndef SFCC_WORKSPACE_HPP_
#define SFCC_WORKSPACE_HPP_

#include <filesystem>
#include <map>
#include <string>

namespace workspace {
  typedef std::map<std::string, std::string> cartridges;
  void traverse(std::filesystem::path path, cartridges& cartridges);
}

#endif // SFCC_WORKSPACE_HPP_ 
