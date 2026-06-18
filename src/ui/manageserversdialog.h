#pragma once
#include "config/config.h"
#include <QDialog>
#include <QList>

class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QScrollArea;
class QSpinBox;

class ManageServersDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ManageServersDialog(const QList<ServerConfig> &servers, QWidget *parent = nullptr);
    QList<ServerConfig> servers();

private:
    void addServer();
    void removeServer();
    void moveUp();
    void moveDown();
    void refreshList();
    void onServerSelected(int row);
    void saveCurrentToModel();
    void loadServerToForm(int row);
    bool nameExists(const QString &name, int excludeRow = -1) const;

    QListWidget        *m_serverList;
    QList<ServerConfig> m_servers;
    int                 m_currentRow{-1};

    QWidget    *m_formPanel{nullptr};
    QCheckBox  *m_disabled{nullptr};
    QLineEdit  *m_name{nullptr};
    QLineEdit  *m_host{nullptr};
    QSpinBox   *m_port{nullptr};
    QCheckBox  *m_ssl{nullptr};
    QCheckBox  *m_websocket{nullptr};
    QLineEdit  *m_nick{nullptr};
    QLineEdit  *m_user{nullptr};
    QLineEdit  *m_realname{nullptr};
    QLineEdit  *m_quitMessage{nullptr};
    QLineEdit  *m_awayMessage{nullptr};
    QLineEdit  *m_password{nullptr};
    QLineEdit  *m_saslUser{nullptr};
    QLineEdit  *m_saslPassword{nullptr};
    QCheckBox  *m_saslExternal{nullptr};
    QLineEdit  *m_clientCert{nullptr};
    QLineEdit  *m_clientKey{nullptr};
    QLineEdit  *m_nickservPassword{nullptr};
    QButtonGroup *m_bouncerGroup{nullptr};
    QLineEdit  *m_bouncerNetwork{nullptr};
    QLabel     *m_bouncerNetworkLabel{nullptr};
    QLineEdit  *m_autoJoin{nullptr};
    QLineEdit  *m_proxyHost{nullptr};
    QSpinBox   *m_proxyPort{nullptr};
    QLineEdit  *m_proxyUser{nullptr};
    QLineEdit  *m_proxyPass{nullptr};

    struct KeychainState {
        bool password{false};
        bool saslPassword{false};
        bool nickservPassword{false};
        bool proxyPass{false};
    };
    QList<KeychainState> m_keychainStates;
};
