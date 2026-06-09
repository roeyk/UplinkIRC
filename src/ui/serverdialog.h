#pragma once
#include "config/config.h"
#include <QDialog>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QCheckBox;

class ServerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ServerDialog(QWidget *parent = nullptr);
    explicit ServerDialog(const ServerConfig &existing, QWidget *parent = nullptr);
    ServerConfig serverConfig() const;

private:
    QLineEdit *m_name;
    QLineEdit *m_host;
    QSpinBox  *m_port;
    QCheckBox *m_ssl;
    QCheckBox *m_websocket;
    QLineEdit *m_nick;
    QLineEdit *m_user;
    QLineEdit *m_realname;
    QLineEdit *m_password;
    QLineEdit   *m_saslUser;
    QLineEdit   *m_saslPassword;
    QCheckBox   *m_saslExternal;
    QLineEdit   *m_clientCert;
    QLineEdit   *m_clientKey;
    QLineEdit   *m_nickservPassword;
    QComboBox *m_bouncerType;
    QLineEdit *m_bouncerNetwork;
    QLabel    *m_bouncerNetworkLabel{nullptr};
    QLineEdit *m_autoJoin;
    QLineEdit *m_proxyHost;
    QSpinBox  *m_proxyPort;
    QLineEdit *m_proxyUser;
    QLineEdit *m_proxyPass;

    bool m_passwordKeychain{false};
    bool m_saslPasswordKeychain{false};
    bool m_nickservPasswordKeychain{false};
    bool m_proxyPassKeychain{false};
};
