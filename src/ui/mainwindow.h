#pragma once

#include <QMainWindow>
#include <QQueue>
#include <QRegularExpression>
#include <QStringList>
#include <QColor>
#include <QHash>
#include <QMap>
#include <QPixmap>
#include <QSet>
#include <QPair>
#include "model/sessionmodel.h"
#include "config/config.h"
#include "ui/themeloader.h"

class CommandDispatcher;
class SidebarDelegate;
class NickDelegate;
class TrayIcon;
class SignalBars;
class AboutDialog;
class ChannelListDialog;
class DocsDialog;
class PreferencesDialog;
class LinkPreview;
class EmojiPicker;
class DccSend;
class DccReceive;
class ChannelPane;
class QNetworkAccessManager;

class ChatView;
class QTimer;
class QTreeWidget;
class QTreeWidgetItem;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QToolButton;
class QSplitter;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(SessionModel *model, const Config &cfg, QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    // Model → UI
    void onServerAdded      (const QString &host);
    void onServerConnected  (const QString &host);
    void onServerDisconnected(const QString &host);
    void onChannelAdded     (const QString &host, const QString &channel);
    void onChannelRemoved   (const QString &host, const QString &channel);
    void onMessageAdded     (const QString &host, const QString &channel, const Message &msg);
    void onTopicChanged     (const QString &host, const QString &channel, const QString &topic);
    void onNickListChanged     (const QString &host, const QString &channel);
    void onNickAdded           (const QString &host, const QString &channel, const QString &nick);
    void onNickRemoved         (const QString &host, const QString &channel, const QString &nick);
    void onNickRenamed         (const QString &host, const QString &channel,
                                const QString &oldNick, const QString &newNick);
    void onNickListContextMenu  (const QPoint &pos);
    void onSidebarContextMenu   (const QPoint &pos);
    void onUnreadChanged    (const QString &host, const QString &channel, int count);
    void onReactionsChanged (const QString &host, const QString &channel, const QString &msgid);
    void onSelfNickChanged  (const QString &host, const QString &nick);
    void onMessageRedacted   (const QString &host, const QString &channel, const QString &msgid);

    // UI → Model
    void onSidebarSelectionChanged();
    void onInputSubmit();
    void dispatchInput(const QString &text, const QString &host, const QString &channel);

    // Typing
    void onTypingReceived(const QString &host, const QString &channel,
                          const QString &nick, const QString &state);

private:
    void setupToolbar();
    void setupSidebar();
    void setupChatArea();
    void setupNickPanel();
    void setupInputBar();
    void correctStartupGeometry();
    void ensureEmojiPicker();
    void connectModel();
    void connectPreferences();

    void switchToChannel(const QString &host, const QString &channel);
    void openChannelList(const QString &host);
    void refreshChatView(const QString &host, const QString &channel, bool resetToLatest = true);
    void loadOlderMessages();
    void refreshNickList(const QString &host, const QString &channel);
    void scheduleNickRefresh(const QString &host, const QString &channel);
    void refreshTopicBar(const QString &host, const QString &channel);
    void appendMessage  (const Message &msg, bool autoPreview = false);
    void applyFontSizes();
    void updateTypingLabel();
    void openChannelPane (const QString &host, const QString &channel);
    void closeChannelPane(const QString &host, const QString &channel);
    ChannelPane *paneAt(const QPoint &globalPos) const;
    bool         isOverPrimary(const QPoint &globalPos) const;
    void refreshPaneChatView(ChannelPane *pane);
    void refreshPaneNickList(ChannelPane *pane);
    void rebuildPaneLayout();

    QListWidgetItem *makeNickItem(const NickEntry &e, const Channel *ch, const ServerSession *sess);
    static int       findNickRow (QListWidget *list, const QString &nick);

    QString    formatMessage(const Message &msg) const;
    void       toggleEventGroupInView(ChatView *view, const QString &groupId,
                                      const QString &host, const QString &channel);
    void       handleChatViewContextMenu(ChatView *view, const QString &anchor,
                                         const QPoint &globalPos,
                                         const QString &host, const QString &channel);
    void       showNickContextMenu(const QString &nick, const QPoint &globalPos);
    QString    msgidAtViewPos(const QPoint &viewPos) const;
    void       doSearch(bool backward);
    void       showSearchBar();
    void       clearReplyBar();

    // Tab completion
    void handleTabComplete(QPlainTextEdit *input, const QString &host, const QString &channel);
    void repositionSendBtn();
    QStringList m_tabCandidates;
    int         m_tabCandidateIndex{0};
    int         m_tabWordStart{0};
    QString     m_tabPrefix;
    bool        m_tabActive{false};

    // Emoji inline autocomplete
    void checkEmojiAutocomplete(const QString &text);
    void commitEmojiAutocomplete(int row);
    void hideEmojiAutocomplete();
    QListWidget *m_emojiCompleter{nullptr};
    int          m_emojiTriggerPos{-1};

    // Input history
    void handleHistoryUp();
    void handleHistoryDown();
    QStringList m_inputHistory;
    int         m_historyIndex{-1};
    QString     m_historyDraft;

    QTreeWidgetItem *findServerItem (const QString &host) const;
    QTreeWidgetItem *findChannelItem(const QString &host, const QString &channel) const;

    // Widgets
    QTreeWidget      *m_sidebar;
    SidebarDelegate  *m_sidebarDelegate{nullptr};
    NickDelegate     *m_nickDelegate{nullptr};
    ChatView *m_chatView;
    QPlainTextEdit *m_input;
    QLabel       *m_nickPrefix;
    QPushButton  *m_emojiBtn;
    QToolButton  *m_sendBtn{nullptr};
    QWidget      *m_sidebarPanel{nullptr};
    QWidget      *m_sidebarHeader{nullptr};
    QToolButton  *m_sidebarToggleBtn{nullptr};
    bool          m_sidebarExpanded{true};
    int           m_sidebarExpandedWidth{180};
    QSplitter    *m_mainSplitter{nullptr};
    QWidget      *m_rightContent{nullptr};
    QWidget      *m_primaryPanel{nullptr};
    QWidget      *m_primaryHeader{nullptr};
    QToolButton  *m_primaryTopicBtn{nullptr};
    QToolButton  *m_searchBtn{nullptr};
    QToolButton  *m_primaryCloseBtn{nullptr};
    QListWidget  *m_nickList;
    QWidget      *m_nickPanel{nullptr};
    QLabel       *m_nickGroupsIconLabel{nullptr};
    QLabel       *m_nickCountLabel{nullptr};
    QToolButton  *m_nickToggleBtn{nullptr};
    QToolButton  *m_nickRevealBtn{nullptr};
    QToolButton  *m_sidebarRevealBtn{nullptr};
    QToolButton  *m_sidebarCloseBtn{nullptr};
    QWidget      *m_chatSection{nullptr};
    QSplitter    *m_chatSplitter{nullptr};
    QSplitter    *m_panesSplitter{nullptr};
    QHash<QString, ChannelPane*> m_panes;        // key: "host|channel_lower"
    QList<ChannelPane*>          m_orderedPanes; // insertion order for layout
    QSet<QString>                m_nickRefreshPending;    // channels with a debounced refresh queued
    QSet<QString>                m_expandedEventGroups;  // groupIds (first-msg timestamp ms) of expanded event batches
    ChannelPane                 *m_dragHighlighted{nullptr};
    int                          m_primarySlot{0}; // position of primary panel in layout order
    bool          m_nickExpanded{true};
    QLabel       *m_topicLabel{nullptr};    // #channel (modes)
    QLabel       *m_userInfoLabel{nullptr}; // * network (in nick panel header)
    QWidget      *m_topicDisplay{nullptr};  // topic text — shown when showTopic
    QLabel       *m_topicText{nullptr};
    QLabel       *m_topicSetByLabel{nullptr};
    QToolButton  *m_hamburger;
    QLabel       *m_appLabel{nullptr};
    QLabel       *m_typingLabel{nullptr};
    QWidget      *m_inputBar{nullptr};
    QWidget      *m_searchBar{nullptr};
    QLineEdit    *m_searchInput{nullptr};
    QWidget      *m_replyBar{nullptr};
    QLabel       *m_replyLabel{nullptr};
    QString       m_pendingReplyMsgid;
    QString       m_pendingReactMsgid;
    QString       m_pendingReactHost;
    QString       m_pendingReactChannel;
    AboutDialog        *m_aboutDialog{nullptr};
    ChannelListDialog  *m_channelListDialog{nullptr};
    DocsDialog         *m_docsDialog{nullptr};
    PreferencesDialog  *m_prefsDialog{nullptr};
    EmojiPicker       *m_emojiPicker{nullptr};

    // Typing indicator state
    QTimer                      *m_typingOutTimer{nullptr};
    bool                         m_typingActive{false};
    QHash<QString, QSet<QString>> m_typingNicks;       // "host|channel" → nicks
    QHash<QString, QTimer*>       m_typingNickTimers;  // "host|channel|nick" → timeout
    QHash<QString, int>           m_botIconIdx;        // lowercased nick → 0 (robot) or 1 (alien)
    QHash<QString, int>           m_renderStart;       // "host\tchannel" → first rendered msg index
    bool                          m_loadingOlder{false};

    // Avatar image cache
    QNetworkAccessManager        *m_avatarNam{nullptr};
    QHash<QString, QPixmap>       m_avatarCache;       // URL → scaled pixmap
    QHash<QString, QString>       m_avatarBase64Cache; // URL → base64(PNG) for tooltip
    QList<QString>                m_avatarCacheOrder;  // FIFO eviction order
    QSet<QString>                 m_avatarFetching;    // in-flight URLs
    void fetchAvatar(const QString &url);
    QString nickTooltip(const QString &nick, const QString &host) const;

    QRegularExpression m_selfNickRe;  // pre-compiled highlight regex for active host's nick
    QRegularExpression m_highlightRe; // extra keyword highlights from config

    SessionModel *m_model;
    TrayIcon     *m_tray{nullptr};
    SignalBars   *m_signalBars{nullptr};
    LinkPreview  *m_linkPreview{nullptr};
    QString       m_hoveredUrl;
    QPoint        m_hoverGlobalPos;
    struct PreviewCtx { QString host, channel, msgid; };
    QHash<QString, PreviewCtx> m_previewChannels; // url → {host, channel, msgid}
    QQueue<QUrl>  m_previewQueue;
    bool          m_previewFetchBusy{false};
    QTimer       *m_previewWatchdog{nullptr};
    void enqueuePreview(const QUrl &url, const QString &host, const QString &channel, const QString &msgid);
    void processPreviewQueue();
    QMap<QString, DccSend*> m_pendingPassiveSends;
    Config        m_config;
    Theme         m_theme;

    bool    m_showNickPrefix{true};
    bool    m_showEmojiBtn{false};
    bool    m_showTopic{true};

    CommandDispatcher *m_dispatcher{nullptr};

};
