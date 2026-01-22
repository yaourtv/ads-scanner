#ifndef logging_hpp
#define logging_hpp

#include <string>

namespace Log {

/** Initialize logging to the given file path. Must be called before info/warn/error.
 *  Returns false if the file could not be opened (caller may exit). */
bool init(const std::string &filePath);

void info(const std::string &msg);
void warn(const std::string &msg);
void error(const std::string &msg);

/** Returns true if init() was successfully called. */
bool isInitialized();

} // namespace Log

#endif /* logging_hpp */
