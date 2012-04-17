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

#include "basicinfo.h"
#include "owncloud.h"

#include <QDebug>
#include <qjson/parser.h>

#include <kpixmapsequenceoverlaypainter.h>
#include <KIO/Job>

BasicInfo::BasicInfo(OwnCloudWizard* parent)
 : QWizardPage(parent)
 , m_validServer(false)
 , m_painter(new KPixmapSequenceOverlayPainter(this))
 , m_wizard(parent)
{
    setupUi(this);
    icon->setPixmap(QIcon::fromTheme("owncloud").pixmap(32, 32));

    int lineEditHeight = server->sizeHint().height();
    QSize workingSize(lineEditHeight, lineEditHeight);
    working->setMinimumSize(workingSize);

    m_painter->setWidget(working);

    connect(server, SIGNAL(textChanged(QString)), this, SLOT(checkServer()));
}

BasicInfo::~BasicInfo()
{

}

bool BasicInfo::validatePage()
{
    if (username->text().isEmpty() || password->text().isEmpty()) {
        return false;
    }

    if (!m_validServer) {
        return false;
    }

    m_wizard->setUsername(username->text());
    m_wizard->setPassword(password->text());
    m_wizard->setServer(m_server);

    return true;
}

void BasicInfo::checkServer()
{
    checkServer(server->text());
}

void BasicInfo::checkServer(const QString &path)
{
    QString fixedUrl;
    if (!path.startsWith("http://")) {
        fixedUrl.append("http://");
        fixedUrl.append(path);
    } else {
        fixedUrl = path;
    }

    KUrl url(fixedUrl);
    m_json.clear();

    url.setFileName("status.php");
    url.setQuery("");

    checkServer(url);
}

void BasicInfo::checkServer(const KUrl& url)
{
    qDebug() << url;
    setResult(false);
    setWorking(true);
    KIO::TransferJob *job = KIO::get(url, KIO::NoReload, KIO::HideProgressInfo);

    connect(job, SIGNAL(data(KIO::Job*,QByteArray)), SLOT(dataReceived(KIO::Job*,QByteArray)));
    connect(job, SIGNAL(finished(KJob*)), this, SLOT(fileChecked(KJob*)));
}


void BasicInfo::figureOutServer(const QString& urlStr)
{
    KUrl url(urlStr);
    if (url.directory(KUrl::AppendTrailingSlash) == "/") {
        setResult(false);
        return;
    }

    m_json.clear();
    url.setFileName("");
    url = url.upUrl();
    url.setFileName("status.php");

    checkServer(url);
}

void BasicInfo::dataReceived(KIO::Job* job, const QByteArray& data)
{
    m_json.append(data);
}

void BasicInfo::fileChecked(KJob* job)
{
    KIO::TransferJob *kJob = qobject_cast<KIO::TransferJob *>(job);
    if (kJob->error()) {
        qDebug() << job->errorString();
        qDebug() << job->errorText();
        figureOutServer(kJob->url().url());
        return;
    }

    QJson::Parser parser;
    QMap <QString, QVariant> map = parser.parse(m_json).toMap();
    if (!map.contains("version")) {
        figureOutServer(kJob->url().url());
        return;
    }

    m_server = kJob->url();
    m_server.setFileName("");
    setResult(true);
}

void BasicInfo::setWorking(bool start)
{
    working->setPixmap(QPixmap());

    if (!start) {
        m_painter->stop();;
        return;
    }

    m_painter->start();
}

void BasicInfo::setResult(bool result)
{
    QString icon;
    if (result) {
        icon = "dialog-ok-apply";
    } else {
        icon = "dialog-close";
    }

    m_validServer = result;
    setWorking(false);
    working->setPixmap(QIcon::fromTheme(icon).pixmap(working->sizeHint()));
}
