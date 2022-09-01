#include "Path.h"

#ifdef Q_OS_WIN
#include <memory>
#include <windows.h>
#endif

namespace util {
QString canonicalizePath(QString path) {
#ifdef Q_OS_WIN
  // Convert from potential 8.3 paths to full paths on Windows
  {
    auto len = GetLongPathNameW((LPCWSTR)path.utf16(), nullptr, 0);
    std::unique_ptr<wchar_t[]> buf{new wchar_t[len]};
    len = GetLongPathNameW((LPCWSTR)path.utf16(), buf.get(), len);
    path = QString::fromWCharArray(buf.get(), len);
  }
#endif
  return path;
}
} // namespace util