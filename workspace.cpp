#include "workspace.hpp"

void workspace::traverse(std::filesystem::path path, cartridges& cartridges) {
  if (path.string().find("cartridge") != std::string::npos) {
    auto parent_path = path.parent_path();
    cartridges.insert({parent_path.filename(), parent_path.string()});
  } else {
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
      if (!entry.is_directory()) continue;
      traverse(entry.path(), cartridges);
    }
  }
}
