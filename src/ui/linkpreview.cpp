#include "linkpreview.h"

#include <QBuffer>
#include <QHostAddress>
#include <QHostInfo>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTextDocument>
#include <memory>

static constexpr int kMaxBytes     = 32768;   // 32 KB
static constexpr int kMaxImgBytes  = 204800;  // 200 KB
static constexpr int kMaxCache     = 50;
static constexpr int kTimeoutMs    = 6000;
static constexpr int kMaxRedirects = 3;

// ── SSRF helpers ──────────────────────────────────────────────────────────────

static bool isForbiddenAddress(const QHostAddress &a)
{
    if (a.isNull())      return true;
    if (a.isLoopback())  return true;
    if (a.isMulticast()) return true;
    if (a.isInSubnet(QHostAddress("10.0.0.0"),    8))   return true;
    if (a.isInSubnet(QHostAddress("172.16.0.0"),  12))  return true;
    if (a.isInSubnet(QHostAddress("192.168.0.0"), 16))  return true;
    if (a.isInSubnet(QHostAddress("169.254.0.0"), 16))  return true;
    if (a.isInSubnet(QHostAddress("127.0.0.0"),   8))   return true;
    if (a.isInSubnet(QHostAddress("::1"),         128)) return true;
    if (a.isInSubnet(QHostAddress("fc00::"),      7))   return true;
    if (a.isInSubnet(QHostAddress("fe80::"),      10))  return true;
    return false;
}

// Returns true if the URL should be blocked without a DNS lookup.
// Handles scheme enforcement, literal private IPs, and well-known private hostnames.
static bool isBlockedBySchemeOrLiteral(const QUrl &url)
{
    const QString scheme = url.scheme().toLower();
    if (scheme != "http" && scheme != "https") return true;

    const QString host = url.host().toLower();
    if (host.isEmpty()) return true;
    if (host == "localhost" || host.endsWith(".local")) return true;

    QHostAddress addr(host);
    if (!addr.isNull())
        return isForbiddenAddress(addr);

    return false;
}

// ── misc helpers ──────────────────────────────────────────────────────────────

static bool isImageUrl(const QUrl &url)
{
    const QString p = url.path().toLower();
    return p.endsWith(".png") || p.endsWith(".jpg") || p.endsWith(".jpeg")
        || p.endsWith(".gif") || p.endsWith(".webp");
}

static QString decodeEntities(const QString &s)
{
    QTextDocument doc;
    doc.setHtml(s);
    return doc.toPlainText();
}

// ── LinkPreview ───────────────────────────────────────────────────────────────

LinkPreview::LinkPreview(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{}

void LinkPreview::fetch(const QUrl &url)
{
    if (m_dnsId) {
        QHostInfo::abortHostLookup(m_dnsId);
        m_dnsId = 0;
    }
    if (m_imgDnsId) {
        QHostInfo::abortHostLookup(m_imgDnsId);
        m_imgDnsId = 0;
    }
    if (m_reply) {
        m_reply->disconnect(this);
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    m_buf.clear();
    m_redirectCount = 0;

    resolveAndFetch(url);
}

void LinkPreview::fetchHover(const QUrl &url)
{
    if (m_hoverDnsId) { QHostInfo::abortHostLookup(m_hoverDnsId); m_hoverDnsId = 0; }
    if (m_hoverReply) {
        m_hoverReply->disconnect(this);
        m_hoverReply->abort();
        m_hoverReply->deleteLater();
        m_hoverReply = nullptr;
    }
    ++m_hoverGen;
    resolveAndFetchHover(url);
}

void LinkPreview::resolveAndFetchHover(const QUrl &url)
{
    if (isBlockedBySchemeOrLiteral(url)) return;

    const QString key = url.toString();
    if (m_cache.contains(key)) {
        emit titleReady(url, m_cache[key].title);
        return;
    }

    const quint64 gen = m_hoverGen;
    m_hoverDnsId = QHostInfo::lookupHost(url.host(), this,
        [this, url, gen](const QHostInfo &info) {
            m_hoverDnsId = 0;
            if (gen != m_hoverGen) return;
            if (info.error() != QHostInfo::NoError) return;
            if (info.addresses().isEmpty()) return;
            for (const QHostAddress &a : info.addresses())
                if (isForbiddenAddress(a)) return;

            QNetworkRequest req(url);
            req.setRawHeader("User-Agent", "WhatsApp/2");
            req.setRawHeader("Accept", "text/html,application/xhtml+xml;q=0.9,*/*;q=0.8");
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                             QNetworkRequest::ManualRedirectPolicy);
            req.setTransferTimeout(kTimeoutMs);

            m_hoverReply = m_nam->get(req);
            QNetworkReply *reply = m_hoverReply;
            auto buf = std::make_shared<QByteArray>();

            connect(reply, &QNetworkReply::readyRead, this, [reply, buf] {
                const qsizetype rem = kMaxBytes - buf->size();
                if (rem <= 0) { reply->abort(); return; }
                buf->append(reply->read(rem));
                if (buf->size() >= kMaxBytes) reply->abort();
            });

            connect(reply, &QNetworkReply::finished, this, [this, reply, buf, url, gen] {
                if (m_hoverReply == reply) m_hoverReply = nullptr;
                reply->deleteLater();
                if (gen != m_hoverGen) return;
                const QString title = extractTitle(*buf);
                if (!title.isEmpty())
                    emit titleReady(url, title);
            });
        });
}

// Resolves the hostname via DNS, verifies every returned address is non-private,
// then calls doPageFetch. Used for both initial fetches and redirect follow-ups.
void LinkPreview::resolveAndFetch(const QUrl &url)
{
    if (isBlockedBySchemeOrLiteral(url)) return;

    if (m_redirectCount == 0) {
        const QString key = url.toString();
        if (m_cache.contains(key)) {
            const CachedCard &c = m_cache[key];
            emit titleReady(url, c.title);
            emit cardReady(url, c.title, c.thumbnail);
            return;
        }
    }

    m_pendingUrl = url;
    ++m_fetchGen;
    const quint64 gen = m_fetchGen;

    m_dnsId = QHostInfo::lookupHost(url.host(), this,
        [this, url, gen](const QHostInfo &info) {
            m_dnsId = 0;
            if (gen != m_fetchGen) return;  // superseded by a newer fetch()
            if (info.error() != QHostInfo::NoError) return;
            if (info.addresses().isEmpty()) return;
            for (const QHostAddress &a : info.addresses())
                if (isForbiddenAddress(a)) return;
            doPageFetch(url);
        });
}

void LinkPreview::doPageFetch(const QUrl &url)
{
    if (isImageUrl(url)) {
        const QString filename = url.fileName();
        fetchImage(url, filename.isEmpty() ? url.host() : filename, url);
        return;
    }

    m_buf.clear();
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "WhatsApp/2");
    req.setRawHeader("Accept", "text/html,application/xhtml+xml;q=0.9,*/*;q=0.8");
    req.setRawHeader("Accept-Language", "en-US,en;q=0.5");
    // Manual policy so we can re-run SSRF checks on each redirect destination.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(kTimeoutMs);

    m_reply = m_nam->get(req);
    QNetworkReply *reply = m_reply;

    connect(reply, &QNetworkReply::readyRead, this, [this, reply] {
        if (m_reply != reply) return;
        const qsizetype rem = kMaxBytes - m_buf.size();
        if (rem <= 0) { reply->abort(); return; }
        m_buf.append(reply->read(rem));
        if (m_buf.size() >= kMaxBytes) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        if (m_reply != reply) return;
        m_reply = nullptr;

        // Re-run DNS check on every redirect destination before following it.
        const QVariant redir =
            reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redir.isValid()) {
            const QUrl next = reply->url().resolved(redir.toUrl());
            reply->deleteLater();
            m_buf.clear();
            if (m_redirectCount < kMaxRedirects) {
                ++m_redirectCount;
                resolveAndFetch(next);
            }
            return;
        }

        const QUrl    pageUrl = m_pendingUrl;
        const QString title   = extractTitle(m_buf);
        const QUrl    imgUrl  = extractImageUrl(m_buf);
        reply->deleteLater();
        m_buf.clear();

        if (title.isEmpty()) return;

        emit titleReady(pageUrl, title);

        if (imgUrl.isValid())
            fetchImage(pageUrl, title, imgUrl);
        else {
            if (m_cache.size() >= kMaxCache) m_cache.erase(m_cache.begin());
            m_cache.insert(pageUrl.toString(), {title, {}});
            emit cardReady(pageUrl, title, {});
        }
    });
}

// DNS-checks the image URL before fetching, same policy as page fetches.
void LinkPreview::fetchImage(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl)
{
    if (isBlockedBySchemeOrLiteral(imageUrl)) return;

    if (m_imgDnsId) {
        QHostInfo::abortHostLookup(m_imgDnsId);
        m_imgDnsId = 0;
    }

    m_imgDnsId = QHostInfo::lookupHost(imageUrl.host(), this,
        [this, pageUrl, title, imageUrl](const QHostInfo &info) {
            m_imgDnsId = 0;
            if (info.error() != QHostInfo::NoError) return;
            if (info.addresses().isEmpty()) return;
            for (const QHostAddress &a : info.addresses())
                if (isForbiddenAddress(a)) return;
            doImageFetch(pageUrl, title, imageUrl);
        });
}

void LinkPreview::doImageFetch(const QUrl &pageUrl, const QString &title, const QUrl &imageUrl)
{
    QNetworkRequest req(imageUrl);
    req.setRawHeader("User-Agent",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/124.0 Safari/537.36");
    // Do not follow image redirects automatically — we can't re-check DNS without
    // a separate redirect counter, and a missing thumbnail is preferable to SSRF.
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::ManualRedirectPolicy);
    req.setTransferTimeout(kTimeoutMs);

    auto *imgReply = m_nam->get(req);
    auto  imgBuf   = std::make_shared<QByteArray>();

    connect(imgReply, &QNetworkReply::readyRead, this, [imgReply, imgBuf] {
        const qsizetype rem = kMaxImgBytes - imgBuf->size();
        if (rem <= 0) { imgReply->abort(); return; }
        imgBuf->append(imgReply->read(rem));
        if (imgBuf->size() >= kMaxImgBytes) imgReply->abort();
    });

    connect(imgReply, &QNetworkReply::finished, this,
        [this, imgReply, imgBuf, pageUrl, title] {
            // Redirect on image URL: emit card without thumbnail rather than following.
            const QVariant redir =
                imgReply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            imgReply->deleteLater();
            if (redir.isValid()) {
                if (m_cache.size() >= kMaxCache) m_cache.erase(m_cache.begin());
                m_cache.insert(pageUrl.toString(), {title, {}});
                emit cardReady(pageUrl, title, {});
                return;
            }

            QPixmap pm;
            if (!imgBuf->isEmpty()) {
                QBuffer buf(imgBuf.get());
                buf.open(QIODevice::ReadOnly);
                QImageReader reader(&buf);
                const QSize srcSize = reader.size();
                if (srcSize.isValid() && srcSize.width() <= 4096 && srcSize.height() <= 4096) {
                    reader.setScaledSize(srcSize.scaled(360, 220, Qt::KeepAspectRatio));
                    QImage img = reader.read();
                    if (!img.isNull())
                        pm = QPixmap::fromImage(std::move(img));
                }
            }

            if (m_cache.size() >= kMaxCache) m_cache.erase(m_cache.begin());
            m_cache.insert(pageUrl.toString(), {title, pm});
            emit cardReady(pageUrl, title, pm);
        });
}

QString LinkPreview::extractTitle(const QByteArray &data) const
{
    const QString html = QString::fromUtf8(data);

    static const QRegularExpression ogPropFirst(
        R"(<meta[^>]+property\s*=\s*["']og:title["'][^>]+content\s*=\s*["']([^"']{1,200})["'])",
        QRegularExpression::CaseInsensitiveOption);
    auto m = ogPropFirst.match(html);
    if (m.hasMatch()) return decodeEntities(m.captured(1).trimmed());

    static const QRegularExpression ogContentFirst(
        R"(<meta[^>]+content\s*=\s*["']([^"']{1,200})["'][^>]+property\s*=\s*["']og:title["'])",
        QRegularExpression::CaseInsensitiveOption);
    m = ogContentFirst.match(html);
    if (m.hasMatch()) return decodeEntities(m.captured(1).trimmed());

    static const QRegularExpression titleTag(
        R"(<title[^>]*>([\s\S]{1,300}?)</title>)",
        QRegularExpression::CaseInsensitiveOption);
    m = titleTag.match(html);
    if (m.hasMatch()) {
        QString t = decodeEntities(m.captured(1).simplified());
        if (!t.isEmpty()) return t;
    }

    return {};
}

QUrl LinkPreview::extractImageUrl(const QByteArray &data) const
{
    const QString html = QString::fromUtf8(data);

    static const QRegularExpression ogPropFirst(
        R"(<meta[^>]+property\s*=\s*["']og:image["'][^>]+content\s*=\s*["']([^"']{1,500})["'])",
        QRegularExpression::CaseInsensitiveOption);
    auto m = ogPropFirst.match(html);
    if (m.hasMatch()) {
        const QUrl u(m.captured(1).trimmed());
        if (u.isValid() && u.scheme().startsWith("http")) return u;
    }

    static const QRegularExpression ogContentFirst(
        R"(<meta[^>]+content\s*=\s*["']([^"']{1,500})["'][^>]+property\s*=\s*["']og:image["'])",
        QRegularExpression::CaseInsensitiveOption);
    m = ogContentFirst.match(html);
    if (m.hasMatch()) {
        const QUrl u(m.captured(1).trimmed());
        if (u.isValid() && u.scheme().startsWith("http")) return u;
    }

    return {};
}
