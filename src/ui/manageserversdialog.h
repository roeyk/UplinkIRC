#pragma once
#include "config/config.h"
#include <QDialog>
#include <QList>

class QListWidget;

class ManageServersDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ManageServersDialog(const QList<ServerConfig> &servers, QWidget *parent = nullptr);
    QList<ServerConfig> servers() const { return m_servers; }

private:
    void addServer();
    void editServer();
    void removeServer();
    void moveUp();
    void moveDown();
    void refreshList();

    QListWidget          *m_list;
    QList<ServerConfig>   m_servers;
};
