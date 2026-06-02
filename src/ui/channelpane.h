#pragma once
#include <QWidget>
#include <QString>
#include <QPoint>

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
    QString        key()     const { return m_host + "|" + m_channel.toLower(); }
    QTextBrowser *chatView() const { return m_chatView; }
    QListWidget  *nickList() const { return m_nickList; }
    void setNick(const QString &nick);
    void setNickVisible(bool visible);
    void setInputFont(const QFont &nickFont, const QFont &inputFont);
    void setTopic(const QString &html);
    void setDragHighlight(bool on);
signals:
    void closeRequested();
    void inputSubmitted(const QString &text);
    void dragActive (const QString &sourceKey, const QPoint &globalPos);
    void dragDropped(const QString &sourceKey, const QPoint &globalPos);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
private:
    QString       m_host;
    QString       m_channel;
    QWidget      *m_header{nullptr};
    QPoint        m_dragStartPos;   // global coords
    bool          m_dragPending{false};
    bool          m_dragging{false};
    QTextBrowser *m_chatView{nullptr};
    QListWidget  *m_nickList{nullptr};
    QLineEdit    *m_input{nullptr};
    QLabel       *m_nickPrefix{nullptr};
    QWidget      *m_topicBar{nullptr};
    QLabel       *m_topicText{nullptr};
    QToolButton  *m_topicToggle{nullptr};
};
