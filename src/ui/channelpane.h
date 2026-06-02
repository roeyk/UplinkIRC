#pragma once
#include <QWidget>
#include <QString>
#include <QPoint>

class QTextBrowser;
class QListWidget;
class QLineEdit;
class QLabel;
class QToolButton;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;

class ChannelPane : public QWidget {
    Q_OBJECT
public:
    explicit ChannelPane(const QString &host, const QString &channel, QWidget *parent = nullptr);
    const QString &host()    const { return m_host; }
    const QString &channel() const { return m_channel; }
    QString        key()     const { return m_host + "|" + m_channel.toLower(); }
    QTextBrowser *chatView() const { return m_chatView; }
    QListWidget  *nickList() const { return m_nickList; }
    void setNick(const QString &nick);
    void setTopic(const QString &html);
signals:
    void closeRequested();
    void inputSubmitted(const QString &text);
    void dropReceived(const QString &sourceKey);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
private:
    QString       m_host;
    QString       m_channel;
    QWidget      *m_header{nullptr};
    QPoint        m_dragStartPos;
    QTextBrowser *m_chatView{nullptr};
    QListWidget  *m_nickList{nullptr};
    QLineEdit    *m_input{nullptr};
    QLabel       *m_nickPrefix{nullptr};
    QWidget      *m_topicBar{nullptr};
    QLabel       *m_topicText{nullptr};
    QToolButton  *m_topicToggle{nullptr};
};
