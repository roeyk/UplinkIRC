#include "stsstore.h"

#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>

static QString stsPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation)
           + "/.config/uplink/sts.ini";
}

bool StsStore::lookup(const QString &host, Policy &out)
{
    QSettings s(stsPath(), QSettings::IniFormat);
    s.beginGroup(host);
    if (!s.contains("expires")) {
        s.endGroup();
        return false;
    }
    const qint64 expires = s.value("expires").toLongLong();
    const qint64 now     = QDateTime::currentSecsSinceEpoch();
    if (expires > 0 && now >= expires) {
        s.endGroup();
        remove(host);
        return false;
    }
    out.port    = static_cast<quint16>(s.value("port", 6697).toUInt());
    out.expires = expires;
    s.endGroup();
    return true;
}

void StsStore::store(const QString &host, quint16 port, qint64 durationSecs)
{
    QSettings s(stsPath(), QSettings::IniFormat);
    s.beginGroup(host);
    s.setValue("port",    static_cast<uint>(port));
    s.setValue("expires", QDateTime::currentSecsSinceEpoch() + durationSecs);
    s.endGroup();
}

void StsStore::remove(const QString &host)
{
    QSettings s(stsPath(), QSettings::IniFormat);
    s.remove(host);
}
