#pragma once

#include <QObject>
#include <QHash>
#include <QUrl>
#include <QPixmap>

class QNetworkAccessManager;
class QNetworkReply;

class LinkPreview : public QObject
{
    Q_OBJECT
public:
    explicit LinkPreview(QObject *parent = nullptr);
    void fetch(const QUrl &url);
    void fetchHover(const QUrl &url);

signals:
    void titleReady(const QUrl &url, const QString &title);
    void cardReady(const QUrl &pageUrl, const QString &title, const QPixmap &thumbnail);

private:
    struct CachedCard {
        QString title;
        QPixmap thumbnail;
    };

    void resolveAndFetch(const QUrl &url);
    void doPageFetch(const QUrl &url);
    void fetchImage(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl);
    void doImageFetch(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl);
    void resolveAndFetchHover(const QUrl &url);

    QString extractTitle(const QByteArray &data) const;
    QUrl    extractImageUrl(const QByteArray &data) const;

    QNetworkAccessManager    *m_nam;

    // Card fetch path
    QNetworkReply            *m_reply{nullptr};
    QUrl                      m_pendingUrl;
    QByteArray                m_buf;
    int                       m_dnsId{0};
    int                       m_imgDnsId{0};
    int                       m_redirectCount{0};
    quint64                   m_fetchGen{0};

    // Hover fetch path (abort-on-new, page only, no image)
    QNetworkReply            *m_hoverReply{nullptr};
    int                       m_hoverDnsId{0};
    quint64                   m_hoverGen{0};

    QHash<QString, CachedCard> m_cache;
};
