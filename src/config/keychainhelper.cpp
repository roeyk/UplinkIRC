#include "keychainhelper.h"

#include <QDebug>
#include <QEventLoop>
#include <qt6keychain/keychain.h>

static const QString kService = QStringLiteral("Uplink");

namespace KeychainHelper {

QString read(const QString &key)
{
    QKeychain::ReadPasswordJob job(kService);
    job.setAutoDelete(false);
    job.setKey(key);
    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (job.error() != QKeychain::NoError)
        return {};
    return job.textData();
}

bool write(const QString &key, const QString &value)
{
    QKeychain::WritePasswordJob job(kService);
    job.setAutoDelete(false);
    job.setKey(key);
    job.setTextData(value);
    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    return job.error() == QKeychain::NoError;
}

void remove(const QString &key)
{
    QKeychain::DeletePasswordJob job(kService);
    job.setAutoDelete(false);
    job.setKey(key);
    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
}

void readAsync(const QString &key, std::function<void(const QString &)> callback)
{
    auto *job = new QKeychain::ReadPasswordJob(kService);
    job->setAutoDelete(false);
    job->setKey(key);
    QObject::connect(job, &QKeychain::Job::finished, job,
        [job, cb = std::move(callback)]() {
            if (job->error() != QKeychain::NoError)
                qWarning() << "Uplink: keychain read failed for" << job->key()
                           << "—" << job->errorString();
            cb(job->error() == QKeychain::NoError ? job->textData() : QString{});
            job->deleteLater();
        });
    job->start();
}

} // namespace KeychainHelper
