#include "linkpreview.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>

static constexpr int kMaxBytes  = 16384;
static constexpr int kMaxCache  = 50;
static constexpr int kTimeoutMs = 4000;

LinkPreview::LinkPreview(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void LinkPreview::fetch(const QUrl &url)
{
    const QString key = url.toString();

    if (m_cache.contains(key)) {
        emit titleReady(url, m_cache.value(key));
        return;
    }

    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    m_pendingUrl = url;
    m_buf.clear();

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "WhatsApp/2");
    req.setTransferTimeout(kTimeoutMs);

    m_reply = m_nam->get(req);

    connect(m_reply, &QNetworkReply::readyRead, this, [this]{
        const int remaining = kMaxBytes - m_buf.size();
        if (remaining <= 0) { m_reply->abort(); return; }
        m_buf.append(m_reply->read(remaining));
        if (m_buf.size() >= kMaxBytes)
            m_reply->abort();
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]{
        const QUrl url   = m_pendingUrl;
        const QString title = extractTitle(m_buf);
        m_reply->deleteLater();
        m_reply = nullptr;
        m_buf.clear();

        if (title.isEmpty()) return;

        if (m_cache.size() >= kMaxCache)
            m_cache.erase(m_cache.begin());
        m_cache.insert(url.toString(), title);

        emit titleReady(url, title);
    });
}

QString LinkPreview::extractTitle(const QByteArray &data) const
{
    const QString html = QString::fromUtf8(data);

    // og:title — property before content
    static const QRegularExpression ogPropFirst(
        R"(<meta[^>]+property\s*=\s*["']og:title["'][^>]+content\s*=\s*["']([^"']{1,200})["'])",
        QRegularExpression::CaseInsensitiveOption);
    auto m = ogPropFirst.match(html);
    if (m.hasMatch()) return m.captured(1).trimmed();

    // og:title — content before property
    static const QRegularExpression ogContentFirst(
        R"(<meta[^>]+content\s*=\s*["']([^"']{1,200})["'][^>]+property\s*=\s*["']og:title["'])",
        QRegularExpression::CaseInsensitiveOption);
    m = ogContentFirst.match(html);
    if (m.hasMatch()) return m.captured(1).trimmed();

    // <title> fallback
    static const QRegularExpression titleTag(
        R"(<title[^>]*>([^<]{1,200})</title>)",
        QRegularExpression::CaseInsensitiveOption);
    m = titleTag.match(html);
    if (m.hasMatch()) return m.captured(1).trimmed();

    return {};
}
