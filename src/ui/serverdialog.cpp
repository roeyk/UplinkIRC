#include "serverdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ServerDialog::ServerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Add Server");
    setMinimumWidth(380);

    m_name     = new QLineEdit;
    m_host     = new QLineEdit;
    m_host->setPlaceholderText("irc.example.org");
    m_port     = new QSpinBox;
    m_port->setRange(1, 65535);
    m_port->setValue(6697);
    m_ssl      = new QCheckBox("Use SSL/TLS");
    m_ssl->setChecked(true);
    m_nick     = new QLineEdit;
    m_user     = new QLineEdit;
    m_realname = new QLineEdit;
    m_password = new QLineEdit;
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("optional");
    m_saslUser         = new QLineEdit;
    m_saslPassword     = new QLineEdit;
    m_saslPassword->setEchoMode(QLineEdit::Password);
    m_saslExternal     = new QCheckBox("Use SASL EXTERNAL (client certificate)");
    m_clientCert       = new QLineEdit;
    m_clientCert->setPlaceholderText("path/to/client.crt (PEM)");
    m_clientKey        = new QLineEdit;
    m_clientKey->setPlaceholderText("path/to/client.key (PEM)");
    m_nickservPassword = new QLineEdit;
    m_nickservPassword->setEchoMode(QLineEdit::Password);
    m_nickservPassword->setPlaceholderText("optional");

    auto makeHeader = [](const QString &text) {
        auto *lbl = new QLabel(text);
        lbl->setStyleSheet("font-weight: bold; margin-top: 6px;");
        return lbl;
    };

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    form->addRow(makeHeader("Connection"));
    form->addRow("Name:",    m_name);
    form->addRow("Host:",    m_host);
    form->addRow("Port:",    m_port);
    form->addRow("",         m_ssl);

    form->addRow(makeHeader("Identity"));
    form->addRow("Nick:",      m_nick);
    form->addRow("Username:",  m_user);
    form->addRow("Real Name:", m_realname);

    auto makeBrowseRow = [](QLineEdit *edit, const QString &filter, QWidget *parent) {
        auto *row    = new QWidget(parent);
        auto *hbox   = new QHBoxLayout(row);
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->addWidget(edit);
        auto *btn = new QPushButton("Browse…", row);
        btn->setFixedWidth(90);
        hbox->addWidget(btn);
        QObject::connect(btn, &QPushButton::clicked, edit, [edit, filter, parent]{
            const QString p = QFileDialog::getOpenFileName(parent, "Select File", {}, filter);
            if (!p.isEmpty()) edit->setText(p);
        });
        return row;
    };

    form->addRow(makeHeader("Authentication"));
    form->addRow("Server Password:", m_password);
    form->addRow("SASL User:",       m_saslUser);
    form->addRow("SASL Password:",   m_saslPassword);
    form->addRow("",                 m_saslExternal);
    form->addRow("Client Cert:",     makeBrowseRow(m_clientCert, "Certificates (*.crt *.pem *.cer);;All (*)", this));
    form->addRow("Client Key:",      makeBrowseRow(m_clientKey,  "Keys (*.key *.pem);;All (*)", this));
    form->addRow("NickServ:",        m_nickservPassword);

    m_autoJoin = new QLineEdit;
    m_autoJoin->setPlaceholderText("#channel1,#channel2");

    m_bouncerType = new QComboBox;
    m_bouncerType->addItem("None",  static_cast<int>(BouncerType::None));
    m_bouncerType->addItem("ZNC",   static_cast<int>(BouncerType::ZNC));
    m_bouncerType->addItem("Soju",  static_cast<int>(BouncerType::Soju));
    m_bouncerNetwork = new QLineEdit;
    m_bouncerNetwork->setPlaceholderText("network name (soju only)");

    auto *channelNote = new QLabel(
        "⚠ To join a password-protected channel, edit <b>config.toml</b> directly:<br/>"
        "<code>&nbsp;&nbsp;[[server.channel]]<br/>"
        "&nbsp;&nbsp;name = \"#private\"<br/>"
        "&nbsp;&nbsp;key &nbsp;= \"secretkey\"</code>");
    channelNote->setWordWrap(true);
    channelNote->setStyleSheet("font-size: 8pt; color: #aaaaaa; margin-top: 2px;");
    channelNote->setTextFormat(Qt::RichText);

    form->addRow(makeHeader("Channels"));
    form->addRow("Auto-join:", m_autoJoin);
    form->addRow("", channelNote);

    form->addRow(makeHeader("Bouncer"));
    form->addRow("Type:",    m_bouncerType);
    form->addRow("Network:", m_bouncerNetwork);

    m_proxyHost = new QLineEdit;
    m_proxyHost->setPlaceholderText("e.g. 127.0.0.1 (leave blank for no proxy)");
    m_proxyPort = new QSpinBox;
    m_proxyPort->setRange(1, 65535);
    m_proxyPort->setValue(1080);
    m_proxyUser = new QLineEdit;
    m_proxyUser->setPlaceholderText("optional");
    m_proxyPass = new QLineEdit;
    m_proxyPass->setPlaceholderText("optional");
    m_proxyPass->setEchoMode(QLineEdit::Password);

    form->addRow(makeHeader("SOCKS5 Proxy"));
    form->addRow("Proxy Host:", m_proxyHost);
    form->addRow("Proxy Port:", m_proxyPort);
    form->addRow("Proxy User:", m_proxyUser);
    form->addRow("Proxy Pass:", m_proxyPass);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

ServerDialog::ServerDialog(const ServerConfig &existing, QWidget *parent)
    : ServerDialog(parent)
{
    setWindowTitle("Edit Server");
    m_name->setText(existing.name);
    m_host->setText(existing.host);
    m_port->setValue(existing.port);
    m_ssl->setChecked(existing.ssl);
    m_nick->setText(existing.nick);
    m_user->setText(existing.user);
    m_realname->setText(existing.realname);
    m_password->setText(existing.password);
    m_saslUser->setText(existing.saslUser);
    m_saslPassword->setText(existing.saslPassword);
    m_saslExternal->setChecked(existing.saslExternal);
    m_clientCert->setText(existing.clientCertFile);
    m_clientKey->setText(existing.clientKeyFile);
    m_nickservPassword->setText(existing.nickservPassword);
    m_bouncerType->setCurrentIndex(static_cast<int>(existing.bouncerType));
    m_bouncerNetwork->setText(existing.bouncerNetwork);
    m_proxyHost->setText(existing.proxyHost);
    m_proxyPort->setValue(existing.proxyPort ? existing.proxyPort : 1080);
    m_proxyUser->setText(existing.proxyUser);
    m_proxyPass->setText(existing.proxyPass);

    QStringList names;
    for (const auto &ch : existing.channels)
        names << ch.name;
    m_autoJoin->setText(names.join(","));
}

ServerConfig ServerDialog::serverConfig() const
{
    ServerConfig sc;
    sc.name             = m_name->text().trimmed();
    sc.host             = m_host->text().trimmed();
    sc.port             = static_cast<quint16>(m_port->value());
    sc.ssl              = m_ssl->isChecked();
    sc.nick             = m_nick->text().trimmed();
    sc.user             = m_user->text().trimmed();
    sc.realname         = m_realname->text().trimmed();
    sc.password         = m_password->text();
    sc.saslUser         = m_saslUser->text().trimmed();
    sc.saslPassword     = m_saslPassword->text();
    sc.saslExternal     = m_saslExternal->isChecked();
    sc.clientCertFile   = m_clientCert->text().trimmed();
    sc.clientKeyFile    = m_clientKey->text().trimmed();
    sc.nickservPassword = m_nickservPassword->text();
    sc.bouncerType      = static_cast<BouncerType>(m_bouncerType->currentData().toInt());
    sc.bouncerNetwork   = m_bouncerNetwork->text().trimmed();
    sc.proxyHost        = m_proxyHost->text().trimmed();
    sc.proxyPort        = static_cast<quint16>(m_proxyPort->value());
    sc.proxyUser        = m_proxyUser->text().trimmed();
    sc.proxyPass        = m_proxyPass->text();
    for (const QString &ch : m_autoJoin->text().split(',', Qt::SkipEmptyParts)) {
        const QString name = ch.trimmed();
        if (!name.isEmpty())
            sc.channels.append({name, {}});
    }
    return sc;
}
