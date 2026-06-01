#pragma once
#include <QString>

namespace StsStore {

struct Policy {
    quint16 port{6697};
    qint64  expires{0}; // Unix epoch seconds
};

bool   lookup(const QString &host, Policy &out);
void   store (const QString &host, quint16 port, qint64 durationSecs);
void   remove(const QString &host);

} // namespace StsStore
