#pragma once

#include <QString>
#include <functional>

namespace KeychainHelper {

// Synchronous — blocks with a nested QEventLoop; use only for user-initiated ops.
QString read(const QString &key);
bool    write(const QString &key, const QString &value);
void    remove(const QString &key);

// Async — starts the keychain job and returns immediately; calls back on the main thread.
void readAsync(const QString &key, std::function<void(const QString &)> callback);

} // namespace KeychainHelper
