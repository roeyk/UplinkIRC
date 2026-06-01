#pragma once

#include <QString>

// Synchronous wrappers around qtkeychain for storing IRC passwords.
// All operations block using a local QEventLoop and must be called
// from the main thread only.
namespace KeychainHelper {

QString read(const QString &key);
bool    write(const QString &key, const QString &value);
void    remove(const QString &key);

} // namespace KeychainHelper
