#pragma once

#include <QObject>
#include <QHash>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

class LinkPreview : public QObject
{
    Q_OBJECT
public:
    explicit LinkPreview(QObject *parent = nullptr);
    void fetch(const QUrl &url);

signals:
    void titleReady(const QUrl &url, const QString &title);

private:
    QString extractTitle(const QByteArray &data) const;

    QNetworkAccessManager *m_nam;
    QNetworkReply         *m_reply{nullptr};
    QUrl                   m_pendingUrl;
    QByteArray             m_buf;
    QHash<QString, QString> m_cache;
};
