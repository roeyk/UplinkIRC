#include "stsstore.h"

#include <QDateTime>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>

static QString stsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/.config/uplink/sts.ini";
}

bool StsStore::lookup(const QString &host, Policy &out)
{
    bool expired = false;
    {
        QSettings s(stsPath(), QSettings::IniFormat);
        s.beginGroup(host.toLower());
        if (!s.contains("expires")) {
            s.endGroup();
            return false;
        }
        const qint64 expires = s.value("expires").toLongLong();
        const qint64 now     = QDateTime::currentSecsSinceEpoch();
        if (expires > 0 && now >= expires) {
            expired = true;
        } else {
            out.port    = static_cast<quint16>(s.value("port", 6697).toUInt());
            out.expires = expires;
        }
        s.endGroup();
    }
    // Destruct QSettings before calling remove() to avoid a double-open write conflict.
    if (expired) {
        remove(host);
        return false;
    }
    return true;
}

void StsStore::store(const QString &host, quint16 port, qint64 durationSecs)
{
    const QString path = stsPath();
    {
        QSettings s(path, QSettings::IniFormat);
        s.beginGroup(host.toLower());
        s.setValue("port",    static_cast<uint>(port));
        s.setValue("expires", QDateTime::currentSecsSinceEpoch() + durationSecs);
        s.endGroup();
    }
    // Restrict STS policy file to owner read/write — same hardening as config/log files.
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
}

void StsStore::remove(const QString &host)
{
    QSettings s(stsPath(), QSettings::IniFormat);
    s.remove(host.toLower());
}
