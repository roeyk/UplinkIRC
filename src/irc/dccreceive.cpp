#include "dccreceive.h"

#include <QFileInfo>
#include <QHostAddress>
#include <QStorageInfo>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

// Maximum file size accepted via DCC receive (2 GiB).
static constexpr qint64 kMaxDccReceiveBytes = 2LL * 1024 * 1024 * 1024;

// Mirrors the SSRF helper in linkpreview.cpp — keep in sync if ranges change.
static bool isPrivateDccAddress(const QHostAddress &a)
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

DccReceive::DccReceive(const QString &savePath, quint32 ip, quint16 port, qint64 filesize,
                       QObject *parent)
    : QObject(parent)
    , m_savePath(savePath)
    , m_file(savePath + ".part")
    , m_ip(ip)
    , m_port(port)
    , m_total(filesize)
{}

DccReceive::~DccReceive()
{
    // Remove any partial file that was never renamed to the final path.
    if (m_file.isOpen()) {
        const QString path = m_file.fileName();
        m_file.close();
        QFile::remove(path);
    }
}

static bool checkTransferPrecon(qint64 total, const QString &savePath, DccReceive *dcc)
{
    if (total > kMaxDccReceiveBytes) {
        emit dcc->error(QString("File too large (%1 MB); max is %2 MB")
            .arg(total / (1024 * 1024))
            .arg(kMaxDccReceiveBytes / (1024 * 1024)));
        return false;
    }
    const QStorageInfo storage(QFileInfo(savePath).absolutePath());
    if (storage.isValid() && storage.bytesAvailable() < total) {
        emit dcc->error(QString("Not enough disk space (need %1 MB, have %2 MB)")
            .arg(total / (1024 * 1024))
            .arg(storage.bytesAvailable() / (1024 * 1024)));
        return false;
    }
    return true;
}

void DccReceive::start()
{
    const QHostAddress peerAddr(m_ip);
    if (isPrivateDccAddress(peerAddr)) {
        emit error("DCC blocked: peer address " + peerAddr.toString() + " is private or reserved");
        return;
    }

    if (!checkTransferPrecon(m_total, m_savePath, this))
        return;

    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error("Cannot create: " + m_file.fileName());
        return;
    }
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead,          this, &DccReceive::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &DccReceive::onSocketError);

    QTimer::singleShot(30000, this, [this]{
        if (m_received == 0 && m_socket->state() != QAbstractSocket::ConnectedState) {
            cancel();
            emit error("Connection timeout");
        }
    });

    m_socket->connectToHost(peerAddr, m_port);
}

bool DccReceive::listenPassive(quint32 expectedIp)
{
    if (!checkTransferPrecon(m_total, m_savePath, this))
        return false;

    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error("Cannot create: " + m_file.fileName());
        return false;
    }
    m_server = new QTcpServer(this);
    if (!m_server->listen(QHostAddress::Any, 0)) {
        emit error("Cannot bind listen port");
        m_file.close();
        return false;
    }

    // Only validate peer IP when the expected address is a known global (non-private) address.
    // With NAT/bouncers the advertised IP may differ from the real source, so skip validation
    // for private addresses rather than blocking legitimate transfers.
    const QHostAddress expected(expectedIp);
    const bool validatePeer = expectedIp != 0 && !isPrivateDccAddress(expected);

    connect(m_server, &QTcpServer::newConnection, this, [this, expected, validatePeer] {
        QTcpSocket *incoming = m_server->nextPendingConnection();
        if (validatePeer) {
            // Reject connections from unexpected peers (race/injection protection).
            if (incoming->peerAddress().toIPv4Address() != expected.toIPv4Address()) {
                incoming->abort();
                incoming->deleteLater();
                return;
            }
        }
        m_socket = incoming;
        m_server->close();
        connect(m_socket, &QTcpSocket::readyRead,          this, &DccReceive::onReadyRead);
        connect(m_socket, &QAbstractSocket::errorOccurred, this, &DccReceive::onSocketError);
    });
    QTimer::singleShot(60000, this, [this]{
        if (!m_socket) {
            cancel();
            emit error("No connection received (timeout)");
        }
    });
    return true;
}

quint16 DccReceive::listenPort() const
{
    return m_server ? m_server->serverPort() : 0;
}

void DccReceive::cancel()
{
    if (m_server) m_server->close();
    if (m_socket) m_socket->abort();
    if (m_file.isOpen()) {
        const QString path = m_file.fileName();
        m_file.close();
        QFile::remove(path);
    }
}

void DccReceive::onReadyRead()
{
    const qint64 remaining = m_total - m_received;
    if (remaining <= 0) {
        m_socket->disconnectFromHost();
        return;
    }
    const QByteArray data = m_socket->read(qMin(m_socket->bytesAvailable(), remaining));
    if (m_file.write(data) != data.size()) {
        emit error("Write failed");
        return;
    }
    m_received += data.size();

    // Send cumulative ACK (4 bytes, big-endian)
    const quint32 ack = qToBigEndian(static_cast<quint32>(m_received));
    m_socket->write(reinterpret_cast<const char *>(&ack), 4);

    emit progress(m_received, m_total);

    if (m_received >= m_total) {
        const QString partPath = m_file.fileName();
        m_file.close();
        m_socket->disconnectFromHost();
        if (QFile::rename(partPath, m_savePath))
            emit finished(m_savePath);
        else
            emit error("Download complete but rename failed: " + m_savePath);
    }
}

void DccReceive::onSocketError()
{
    const QString msg = m_socket ? m_socket->errorString() : QString("Unknown socket error");
    if (m_socket)
        m_socket->disconnect(this);  // prevent re-entry from abort() in cancel()
    cancel();
    emit error(msg);
}
