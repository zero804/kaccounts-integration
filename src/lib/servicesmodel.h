/*************************************************************************************
 *  Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *  Copyright (C) 2020 by Dan Leinir Turthra Jensen <admin@leinir.dk>                *
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

#ifndef SERVICES_MODEL_H
#define SERVICES_MODEL_H

#include "kaccounts_export.h"

#include <QAbstractListModel>

/**
 * @brief A model which represents the services in a single account
 *
 * You can create this manually, but usually you would get an instance of it from the
 * AccountsModel::Roles::ServicesRole data role (model.services) of an AccountsModel
 * instance.
 *
 * # Roles
 *
 * The following role names are available in this model:
 *
 * * name: The internal name for the service, as understood by the backend
 * * description: A (usually single line) description of the service
 * * displayName: A human-readable name for the service (use this in the UI instead of name)
 * * serviceType: A machine-readable category for the service (e.g. microblogging, e-mail, IM...)
 * * providerName: The machine-readable name of the provider which this service is related to
 * * iconName: An XDG Icon specification icon name representing this service
 * * tags: A list of strings representing tags set on this service
 * * enabled: Whether or not the service is enabled for the given account
 */
class KACCOUNTS_EXPORT ServicesModel : public QAbstractListModel
{
    Q_OBJECT
    /**
     * The Accounts::Account instance which this model should use for fetching the list of services
     */
    Q_PROPERTY(QObject* account READ account WRITE setAccount NOTIFY accountChanged)
    /**
     * The internal ID for the account this model represents
     */
    Q_PROPERTY(quint32 accountId READ accountId NOTIFY accountChanged)
    /**
     * The human-readable name of the account this model represents
     */
    Q_PROPERTY(QString accountDisplayName READ accountDisplayName NOTIFY accountChanged)
    /**
     * The name of the provider this model's account is signed in through
     */
    Q_PROPERTY(QString accountProviderName READ accountProviderName NOTIFY accountChanged)
    /**
     * The XDG Icon specification icon name for this model's account
     */
    Q_PROPERTY(QString accountIconName READ accountIconName NOTIFY accountChanged)
    public:
        enum Roles {
            NameRole = Qt::UserRole + 1,
            DescriptionRole,
            DisplayNameRole,
            ServiceTypeRole,
            ProviderNameRole,
            IconNameRole,
            TagsRole,
            EnabledRole
        };
        explicit ServicesModel(QObject* parent = nullptr);
        virtual ~ServicesModel();

        QHash< int, QByteArray > roleNames() const override;
        virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        void setAccount(QObject* account);
        QObject* account() const;
        quint32 accountId() const;
        QString accountDisplayName() const;
        QString accountProviderName() const;
        QString accountIconName() const;
        Q_SIGNAL void accountChanged();

    private:
        class Private;
        Private *d;
};

#endif//SERVICES_MODEL_H
