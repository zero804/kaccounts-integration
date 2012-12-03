/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "accountsmodel.h"

#include <QtCore/QDebug>

#include <QtGui/QIcon>
#include <klocalizedstring.h>

#include <Accounts/Account>
#include <Accounts/Manager>

class AccountsModelPrivate : public QObject
{
    public:
        AccountsModelPrivate(AccountsModel *model);
        void insertCreateAccount();
        QVariant createAccountData(int role);
        Accounts::Account* accountById(int id);

        Accounts::Manager *m_manager;
        Accounts::AccountIdList m_accIdList;
        QHash<int, Accounts::Account*> m_accHash;

    private:
        AccountsModel* q;
};

AccountsModelPrivate::AccountsModelPrivate(AccountsModel *model)
 : q(model)
 , m_manager(new Accounts::Manager(this))
{
    m_accIdList = m_manager->accountList();
    m_accIdList.append(0); //For the dummy account Create
}

QVariant AccountsModelPrivate::createAccountData(int role)
{
    if (role == Qt::DisplayRole) {
        return i18n("Create");
    }

    if (role == Qt::DecorationRole) {
        return QIcon::fromTheme("list-add");
    }

//     qDebug() << "CreateAccountData: ";
//     qDebug() << "\tUnimplemented role: " << role;

    return QVariant();
}

Accounts::Account* AccountsModelPrivate::accountById(int id)
{
//     qDebug() << "AccountsModelPrivate::accountById: " << id;
    if (m_accHash.contains(id)) {
//         qDebug() << "\t cached";
        return m_accHash.value(id);
    }

    Accounts::Account* account = m_manager->account(id);
    if (!account) {
        qDebug() << "\t Failed to get the account from manager";
        return 0;
    }

    m_accHash[id] = account;
    return account;
}

AccountsModel::AccountsModel(QObject* parent)
 : QAbstractListModel(parent)
 , d(new AccountsModelPrivate(this))
{
    connect(d->m_manager, SIGNAL(accountCreated(Accounts::AccountId)), SLOT(accountCreated(Accounts::AccountId)));
    connect(d->m_manager, SIGNAL(accountRemoved(Accounts::AccountId)), SLOT(accountRemoved(Accounts::AccountId)));
    connect(d->m_manager, SIGNAL(accountUpdated(Accounts::AccountId)), SLOT(accountUpdated(Accounts::AccountId)));
}

AccountsModel::~AccountsModel()
{

}

int AccountsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return d->m_accIdList.count();
}

QVariant AccountsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    return i18n("Accounts");
}

QVariant AccountsModel::data(const QModelIndex& index, int role) const
{
//     qDebug() << "AccountsModel::data: " << index.row();
    if(!index.isValid()) {
//         qDebug() << "\t Invalid index";
        return QVariant();
    }

    if (index.row() >= d->m_accIdList.count()) {
//         qDebug() << "\t out of range index";
        return QVariant();
    }

    Accounts::AccountId accountId = d->m_accIdList.value(index.row());
    if (accountId == 0) {
        return d->createAccountData(role);
    }

    Accounts::Account *account = d->accountById(accountId);
    if (role == Qt::DisplayRole) {
        return account->displayName();
    }

    if (role == Qt::DecorationRole) {
        QIcon icon = QIcon::fromTheme(d->m_manager->provider(account->providerName()).iconName());
        if (!icon.isNull()) {
            return icon;
        }

        return QIcon::fromTheme("system-help");
    }

//     qDebug() << "data: ";
//     qDebug() << "\tUnimplemented role: " << role;
    return QVariant();
}

bool AccountsModel::removeRows(int row, int count, const QModelIndex& parent)
{
    endRemoveRows();
    return true;
}

void AccountsModel::accountCreated(Accounts::AccountId accountId)
{
    qDebug() << "AccountsModel::accountCreated: " << accountId;
    int row = d->m_accIdList.count();
    if (accountId) {
        row -= 1; // last item will always be the dummy create account
    }

    beginInsertRows(QModelIndex(), row, row);
    d->m_accIdList.insert(row, accountId);
    endInsertRows();
}

void AccountsModel::accountRemoved(Accounts::AccountId accountId)
{
    qDebug() << "AccountsModel::accountRemoved: " << accountId;
    beginRemoveRows(QModelIndex(), d->m_accIdList.indexOf(accountId), d->m_accIdList.indexOf(accountId));
    d->m_accIdList.removeOne(accountId);
    endRemoveRows();
}

void AccountsModel::accountUpdated(Accounts::AccountId accountId)
{
    qDebug() << "Account updated: " << accountId;
}