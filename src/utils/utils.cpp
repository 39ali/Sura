#include "utils.h"

namespace Sura {

std::string Utils::readFile(const char* fileName) {
  FILE* file = fopen(fileName, "r");

  if (!file) {
  }

  return {};
}

}  // namespace Sura
