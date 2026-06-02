#pragma once
#include <QWidget>
#include <QString>

class QTextBrowser;
class QListWidget;
class QLineEdit;
class QLabel;
class QToolButton;

class ChannelPane : public QWidget {
    Q_OBJECT
public:
    explicit ChannelPane(const QString &host, const QString &channel, QWidget *parent = nullptr);
    const QString &host()    const { return m_host; }
    const QString &channel() const { return m_channel; }
    QTextBrowser *chatView() const { return m_chatView; }
    QListWidget  *nickList() const { return m_nickList; }
    void setNick(const QString &nick);
    void setTopic(const QString &html);
signals:
    void closeRequested();
    void inputSubmitted(const QString &text);
private:
    QString       m_host;
    QString       m_channel;
    QTextBrowser *m_chatView{nullptr};
    QListWidget  *m_nickList{nullptr};
    QLineEdit    *m_input{nullptr};
    QLabel       *m_nickPrefix{nullptr};
    QWidget      *m_topicBar{nullptr};
    QLabel       *m_topicText{nullptr};
    QToolButton  *m_topicToggle{nullptr};
};
