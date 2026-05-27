#include "mainwindow.h"
#include "irc/ircclient.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_client(new IrcClient(this))
{
    setWindowTitle("UplinkIRC");
    resize(1100, 700);
    setupUi();
    setupMenuBar();

    connect(m_client, &IrcClient::messageReceived, this, &MainWindow::onMessageReceived);
    connect(m_client, &IrcClient::connected,       this, &MainWindow::onConnected);
    connect(m_client, &IrcClient::disconnected,    this, &MainWindow::onDisconnected);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi()
{
    m_splitter   = new QSplitter(Qt::Horizontal, this);
    m_channelList = new QListWidget;
    m_chatView    = new QTextEdit;
    m_nickList    = new QListWidget;
    m_inputBar    = new QLineEdit;

    m_chatView->setReadOnly(true);
    m_channelList->setMaximumWidth(180);
    m_nickList->setMaximumWidth(140);

    auto *center = new QWidget;
    auto *vbox   = new QVBoxLayout(center);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(2);
    vbox->addWidget(m_chatView);
    vbox->addWidget(m_inputBar);

    m_splitter->addWidget(m_channelList);
    m_splitter->addWidget(center);
    m_splitter->addWidget(m_nickList);
    m_splitter->setStretchFactor(1, 1);

    setCentralWidget(m_splitter);
    statusBar()->showMessage("Not connected");

    connect(m_inputBar, &QLineEdit::returnPressed, this, &MainWindow::onInputSubmit);
}

void MainWindow::setupMenuBar()
{
    auto *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction("&Connect",    this, []{ /* TODO */ });
    fileMenu->addAction("&Disconnect", this, []{ /* TODO */ });
    fileMenu->addSeparator();
    fileMenu->addAction("&Quit", qApp, &QApplication::quit);

    auto *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction("&About UplinkIRC", this, []{ /* TODO */ });
}

void MainWindow::onInputSubmit()
{
    const QString text = m_inputBar->text().trimmed();
    if (text.isEmpty()) return;
    m_inputBar->clear();
    // TODO: route to active channel / command parser
}

void MainWindow::onMessageReceived(const QString &, const QString &, const QString &nick, const QString &text)
{
    m_chatView->append(QString("<%1> %2").arg(nick, text));
}

void MainWindow::onConnected(const QString &server)
{
    statusBar()->showMessage("Connected to " + server);
}

void MainWindow::onDisconnected(const QString &server)
{
    statusBar()->showMessage("Disconnected from " + server);
}
