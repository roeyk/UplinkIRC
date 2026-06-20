#include "manageserversdialog.h"
#include "serverdialog.h"
#include "ui/pillbutton.h"
#include "menuicons.h"

#include <QDialogButtonBox>
#include <QMessageBox>
#include <QPushButton>
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
    auto *btnUp     = new PillButton("▲");
    auto *btnDown   = new PillButton("▼");

    connect(btnAdd,    &QPushButton::clicked, this, &ManageServersDialog::addServer);
    connect(btnEdit,   &QPushButton::clicked, this, &ManageServersDialog::editServer);
    connect(btnRemove, &QPushButton::clicked, this, &ManageServersDialog::removeServer);
    connect(btnUp,     &QPushButton::clicked, this, &ManageServersDialog::moveUp);
    connect(btnDown,   &QPushButton::clicked, this, &ManageServersDialog::moveDown);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &ManageServersDialog::editServer);

    auto *btnBar = new QHBoxLayout;
    btnBar->addWidget(btnAdd);
    btnBar->addWidget(btnEdit);
    btnBar->addWidget(btnRemove);
    btnBar->addStretch();
    btnBar->addWidget(btnUp);
    btnBar->addWidget(btnDown);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setIcon(MenuIcons::confirm());
    buttons->button(QDialogButtonBox::Cancel)->setIcon(MenuIcons::close());
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
    else if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

bool ManageServersDialog::nameExists(const QString &name, int excludeRow) const
{
    for (int i = 0; i < m_servers.size(); ++i) {
        if (i == excludeRow) continue;
        if (m_servers[i].name.compare(name, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

void ManageServersDialog::addServer()
{
    ServerDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    ServerConfig sc = dlg.serverConfig();
    if (sc.host.isEmpty()) return;
    if (nameExists(sc.name)) {
        QMessageBox::warning(this, "Duplicate Name",
            QString("A server named \"%1\" already exists.").arg(sc.name));
        return;
    }
    m_servers.append(sc);
    refreshList();
    m_list->setCurrentRow(static_cast<int>(m_servers.size()) - 1);
}

void ManageServersDialog::editServer()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_servers.size()) return;
    ServerDialog dlg(m_servers[row], this);
    if (dlg.exec() != QDialog::Accepted) return;
    ServerConfig sc = dlg.serverConfig();
    if (sc.host.isEmpty()) return;
    if (nameExists(sc.name, row)) {
        QMessageBox::warning(this, "Duplicate Name",
            QString("A server named \"%1\" already exists.").arg(sc.name));
        return;
    }
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

void ManageServersDialog::moveUp()
{
    const int row = m_list->currentRow();
    if (row <= 0 || row >= m_servers.size()) return;
    m_servers.swapItemsAt(row, row - 1);
    refreshList();
    m_list->setCurrentRow(row - 1);
}

void ManageServersDialog::moveDown()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_servers.size() - 1) return;
    m_servers.swapItemsAt(row, row + 1);
    refreshList();
    m_list->setCurrentRow(row + 1);
}
