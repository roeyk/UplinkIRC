#pragma once
#include <QDialog>
#include "config/config.h"

class QButtonGroup;
class QCheckBox;
class QLineEdit;

class PreferencesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PreferencesDialog(const Config &cfg, QWidget *parent = nullptr);

signals:
    void themeChanged(const QString &name);
    void fontConfigRequested();
    void appIconChanged(const QString &key);
    void topicBarToggled(bool on);
    void nickPrefixToggled(bool on);
    void emojiBtnToggled(bool on);
    void sendBtnToggled(bool on);
    void typingIndicatorToggled(bool on);
    void notificationsToggled(bool on);
    void coloredNicksToggled(bool on);
    void hangingIndentToggled(bool on);
    void loggingToggled(bool on);
    void linkPreviewsToggled(bool on);
    void unreadCountsToggled(bool on);
    void timestampsToggled(bool on);
    void highlightWordsChanged(const QString &words);
    void nickBracketsChanged(const QString &brackets);
    void manageServersRequested();
    void aboutRequested();
    void docsRequested();
    void profileSetRequested(const QString &displayName, const QString &avatarUrl);

private:
    QCheckBox *m_topicCheck{nullptr};
    QCheckBox *m_nickPrefixCheck{nullptr};
    QCheckBox *m_emojiCheck{nullptr};
    QCheckBox *m_sendBtnCheck{nullptr};
    QCheckBox *m_typingCheck{nullptr};
    QCheckBox *m_notificationsCheck{nullptr};
    QCheckBox *m_coloredNicksCheck{nullptr};
    QCheckBox *m_hangingIndentCheck{nullptr};
    QCheckBox *m_loggingCheck{nullptr};
    QCheckBox *m_linkPreviewsCheck{nullptr};
    QCheckBox *m_unreadCountsCheck{nullptr};
    QCheckBox *m_timestampsCheck{nullptr};
    QLineEdit *m_highlightWordsEdit{nullptr};
    QButtonGroup *m_bracketsGroup{nullptr};
    QLineEdit *m_displayNameEdit{nullptr};
    QLineEdit *m_avatarUrlEdit{nullptr};

    static const QList<QPair<QString,QString>> s_iconChoices;
    static const QList<QPair<QString,QString>> s_bracketChoices;
};
