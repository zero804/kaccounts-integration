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

#ifndef RUNNERID_H
#define RUNNERID_H

#include <QtGui/QWizard>

#include <kurl.h>

class RunnerIDWizard : public QWizard
{
    Q_OBJECT
    public:
        explicit RunnerIDWizard(QWidget* parent = 0, Qt::WindowFlags flags = 0);
        virtual ~RunnerIDWizard();

        virtual void done(int result);

        void setUsername(const QString &username);
        void setPassword(const QString &password);

        const QString username() const;
        const QString password() const;

        void activateOption(const QString& name, bool checked);

    Q_SIGNALS:
        void newAccount(const QString &type, const QString &name);

    private:
        QString m_username;
        QString m_password;

        QHash<QString, int> m_services;
};

#endif //OWNCLOUD_H