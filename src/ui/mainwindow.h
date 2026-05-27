#pragma once

#include <QMainWindow>
#include <QSplitter>
#include <QListWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include "config/config.h"

class IrcClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(const Config &cfg, const QString &cfgPath, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onInputSubmit();
    void onMessageReceived(const QString &server, const QString &channel, const QString &nick, const QString &text);
    void onConnected(const QString &server);
    void onDisconnected(const QString &server);

private:
    void setupUi();
    void setupMenuBar();

    QSplitter   *m_splitter;
    QListWidget *m_channelList;
    QTextEdit   *m_chatView;
    QLineEdit   *m_inputBar;
    QListWidget *m_nickList;

    IrcClient   *m_client;
    Config       m_config;
    QString      m_configPath;
};
