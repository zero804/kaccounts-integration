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

#ifndef FOAUTH_H
#define FOAUTH_H

#include "ui_foauth.h"

#include <QWizardPage>

namespace KIO {
    class Job;
};
class KJob;
class FacebookWizard;
class KPixmapSequenceOverlayPainter;
class FOauth : public QWizardPage, Ui::FOAuth
{
    Q_OBJECT
    public:
        explicit FOauth(FacebookWizard* parent);
        virtual ~FOauth();

        virtual bool validatePage();
        virtual void initializePage();
        virtual bool isComplete() const;

    private Q_SLOTS:
        void authenticated(const QString &accessToken);
        void error();
        void gotUsername(KIO::Job* job, const QByteArray &data);
        void usernameFinished();
        void tryAgainSlot();

    private:
        void checkUsername();

    private:
        QByteArray m_json;
        FacebookWizard *m_wizard;
        KPixmapSequenceOverlayPainter *m_painter;
        bool m_valid;
};

#endif //FOAUTH_H