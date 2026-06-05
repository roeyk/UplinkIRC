#pragma once
#include "config/config.h"
#include <QObject>

class SessionModel;
class QWidget;

class CommandDispatcher : public QObject {
    Q_OBJECT
public:
    explicit CommandDispatcher(SessionModel *model, Config *config,
                               QWidget *dialogParent, QObject *parent = nullptr);

    // Returns true if a slash command was handled.
    // replyMsgid is the pending reply target (may be empty).
    bool dispatch(const QString &text, const QString &host, const QString &channel,
                  const QString &replyMsgid);

signals:
    void switchChannel(const QString &host, const QString &channel);
    void focusInput();
    void clearChat();
    void replyBarCleared();

private:
    SessionModel *m_model;
    Config       *m_config;
    QWidget      *m_dialogParent;
    QString       m_sysinfoCache;
    bool          m_sysinfoLoading{false};
};
