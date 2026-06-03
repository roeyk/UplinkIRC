#include "manageserversdialog.h"
#include "serverdialog.h"
#include "ui/pillbutton.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

ManageServersDialog::ManageServersDialog(const QList<ServerConfig> &servers, QWidget *parent)
    : QDialog(parent)
    , m_servers(servers)
{
    setWindowTitle("Manage Servers");
    setMinimumSize(320, 300);

    m_list = new QListWidget;
    refreshList();

    auto *btnAdd    = new PillButton("Add");
    auto *btnEdit   = new PillButton("Edit");
    auto *btnRemove = new PillButton("Remove");

    connect(btnAdd,    &QPushButton::clicked, this, &ManageServersDialog::addServer);
    connect(btnEdit,   &QPushButton::clicked, this, &ManageServersDialog::editServer);
    connect(btnRemove, &QPushButton::clicked, this, &ManageServersDialog::removeServer);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &ManageServersDialog::editServer);

    auto *btnBar = new QHBoxLayout;
    btnBar->addWidget(btnAdd);
    btnBar->addWidget(btnEdit);
    btnBar->addWidget(btnRemove);
    btnBar->addStretch();

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_list);
    layout->addLayout(btnBar);
    layout->addWidget(buttons);
}

void ManageServersDialog::refreshList()
{
    const int cur = m_list->currentRow();
    m_list->clear();
    for (const ServerConfig &sc : m_servers) {
        const QString label = sc.name.isEmpty() ? sc.host : sc.name + "  (" + sc.host + ")";
        m_list->addItem(label);
    }
    if (cur >= 0 && cur < m_list->count())
        m_list->setCurrentRow(cur);
    else if (!m_list->count() == 0)
        m_list->setCurrentRow(0);
}

void ManageServersDialog::addServer()
{
    ServerDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    ServerConfig sc = dlg.serverConfig();
    if (sc.host.isEmpty()) return;
    m_servers.append(sc);
    refreshList();
    m_list->setCurrentRow(m_servers.size() - 1);
}

void ManageServersDialog::editServer()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_servers.size()) return;
    ServerDialog dlg(m_servers[row], this);
    if (dlg.exec() != QDialog::Accepted) return;
    ServerConfig sc = dlg.serverConfig();
    if (sc.host.isEmpty()) return;
    m_servers[row] = sc;
    refreshList();
}

void ManageServersDialog::removeServer()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_servers.size()) return;
    m_servers.removeAt(row);
    refreshList();
}
