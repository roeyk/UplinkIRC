#include "dccreceive.h"

#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>
#include <QtEndian>

DccReceive::DccReceive(const QString &savePath, quint32 ip, quint16 port, qint64 filesize,
                       QObject *parent)
    : QObject(parent)
    , m_file(savePath)
    , m_ip(ip)
    , m_port(port)
    , m_total(filesize)
{}

void DccReceive::start()
{
    if (!m_file.open(QIODevice::WriteOnly)) {
        emit error("Cannot create: " + m_file.fileName());
        return;
    }
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead,          this, &DccReceive::onReadyRead);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &DccReceive::onSocketError);

    QTimer::singleShot(30000, this, [this]{
        if (m_received == 0 && m_socket->state() != QAbstractSocket::ConnectedState)
            emit error("Connection timeout");
    });

    m_socket->connectToHost(QHostAddress(m_ip), m_port);
}

void DccReceive::onReadyRead()
{
    const QByteArray data = m_socket->readAll();
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
        m_file.close();
        m_socket->disconnectFromHost();
        emit finished(m_file.fileName());
    }
}

void DccReceive::onSocketError()
{
    emit error(m_socket->errorString());
}
