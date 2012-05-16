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

#include "webaccounts.h"
#include "create.h"
#include "accountwidget.h"
#include "ui_kcm.h"
#include "google/jobs/createcontact.h"
#include "google/jobs/createcalendar.h"
#include "google/jobs/createtask.h"
#include "google/jobs/createmail.h"
#include "google/jobs/createchat.h"
#include "google/jobs/removechat.h"
#include "google/jobs/removeemail.h"
#include "google/jobs/removecalendar.h"
#include "google/jobs/removetask.h"
#include "jobs/removeakonadiresource.h"

#include "facebook/faccountwidget.h"
#include "facebook/jobs/fcreatechat.h"
#include "facebook/jobs/fcreatepim.h"

#include "owncloud/oaccountwidget.h"
#include "owncloud/jobs/ocreatefile.h"
#include "owncloud/jobs/ocreatecontact.h"
#include "owncloud/jobs/ocreatecalendar.h"
#include "owncloud/jobs/oremovecalendar.h"
#include "owncloud/jobs/oremovecontact.h"
#include "owncloud/jobs/oremovefile.h"

#include <QtGui/QLabel>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QAction>
#include <QtGui/QStackedLayout>

#include <KDebug>
#include <kpluginfactory.h>
#include <kstandarddirs.h>

#include <KWallet/Wallet>

using namespace KWallet;

K_PLUGIN_FACTORY(WebAccountsFactory, registerPlugin<WebAccounts>();)
K_EXPORT_PLUGIN(WebAccountsFactory("webaccounts", "webaccounts"))

WebAccounts::WebAccounts(QWidget *parent, const QVariantList&)
: KCModule(WebAccountsFactory::componentData(), parent)
, m_create(0)
, m_layout(new QStackedLayout)
{

    m_ui = new Ui::KCMWebAccounts();
    m_ui->setupUi(this);

    m_ui->accountInfo->setLayout(m_layout);
    m_ui->accList->setIconSize(QSize(48, 48));

    connect(m_ui->remoteBtn, SIGNAL(clicked(bool)), this, SLOT(rmBtnClicked()));
    connect(m_ui->addBtn, SIGNAL(clicked(bool)), this, SLOT(addBtnClicked()));
    connect(m_ui->accList, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(currentItemChanged(QListWidgetItem*,QListWidgetItem*)));

    QMetaObject::invokeMethod(this, "addExistingAccounts", Qt::QueuedConnection);
}

WebAccounts::~WebAccounts()
{
    delete m_ui;
}

void WebAccounts::addExistingAccounts()
{
    KConfigGroup accGroup = accounts();
    QStringList accountTypes = accGroup.groupList();
    kDebug() << "Existing accounts: " << accountTypes;
    Q_FOREACH(const QString &accType, accountTypes) {
        QStringList accountList = accGroup.group(accType).groupList();
        Q_FOREACH(const QString &account, accountList) {
            addAccount(account, account, accType);
        }
    }

    m_create = new Create(this);
    connect(m_create, SIGNAL(newAccount(QString,QString)), this, SLOT(newAccount(QString,QString)));

    QWidget *widget = m_create->widget();
    m_newAccountItem = createQListWidgetItem(i18n("New Account"), i18n("Select a supported Web Account"), "new", widget);

    m_layout->addWidget(widget);

    m_ui->accList->addItem(m_newAccountItem);

    if (m_ui->accList->count() == 0) {
        m_ui->accList->setCurrentItem(m_newAccountItem);
        return;
    }

    m_ui->accList->setCurrentItem(m_ui->accList->item(0));
}

void WebAccounts::addAccount(const QString& name, const QString& accountName, const QString& type)
{
    QWidget *accountWidget = 0;
    if (type == "google") {
        accountWidget = new AccountWidget(account(name, type), this);
    } else if (type == "facebook") {
        accountWidget = new FAccountWidget(account(name, type), this);
    } else if (type == "owncloud" || type == "runnerid") {
        accountWidget = new OAccountWidget(account(name, type), this);
    }

    m_layout->addWidget(accountWidget);

    QListWidgetItem *newItem = createQListWidgetItem(name, name, type, accountWidget);
    m_ui->accList->addItem(newItem);
}

void WebAccounts::addBtnClicked()
{
    m_ui->accList->setCurrentItem(m_newAccountItem, QItemSelectionModel::SelectCurrent);
}

void WebAccounts::rmBtnClicked()
{
    QListWidgetItem *item = m_ui->accList->currentItem();
    if (item == m_newAccountItem) {
        return;
    }

    QString accName = item->data(Qt::UserRole).toString();
    QString type = item->data(Qt::UserRole + 2).toString();

    KConfigGroup group = account(accName, type);

    if (type == "google") {
        removeGoogleAccount(group);
    } else if(type == "facebook") {
        removeFacebookACcount(group);
    } else if(type == "owncloud" || type == "runnerid") {
        removeOwncloudAccount(group);
    }

    removeAccountIfPossible(accName, type);
    m_ui->accList->takeItem(m_ui->accList->row(item));

    delete item->data(Qt::UserRole + 1).value<QWidget *>();
}

void WebAccounts::serviceRemoved(KJob *job)
{
    QString accName = job->objectName();
    QString type = job->property("type").toString();

    removeAccountIfPossible(accName, type);
}

void WebAccounts::removeAccountIfPossible(const QString& name, const QString& type)
{
    KConfigGroup services = account(name, type).group("services");
    QStringList keys = services.keyList();
    bool deleteAccount = true;
    Q_FOREACH(const QString &key, keys) {
        if (services.readEntry(key, 0) > 0) {
            deleteAccount = false;
            break;
        }
    }

    if (!deleteAccount) {
        return;
    }

    KSharedConfig::openConfig("webaccounts")->group("accounts").group(type).deleteGroup(name);
    KSharedConfig::openConfig("webaccounts")->sync();

    Wallet *wallet = Wallet::openWallet(Wallet::NetworkWallet(), 0, Wallet::Synchronous);
    wallet->setFolder("WebAccounts");

    QString eh = type + "-" + name;
    qDebug() << "REmoving: " << eh;
    wallet->removeEntry(type + "-" + name);
}

void WebAccounts::currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current) {
        return;
    }

    m_ui->accountInfo->setTitle(current->data(Qt::UserRole).toString());
    m_layout->setCurrentWidget(current->data(Qt::UserRole + 1).value<QWidget *>());

    m_ui->remoteBtn->setEnabled(current != m_newAccountItem);
}

void WebAccounts::newAccount(const QString& type, const QString& name)
{
    QWidget *accountWidget = 0;
    if (type == "google") {
        accountWidget = new AccountWidget(account(name, type), this);
    } else if (type == "facebook") {
        accountWidget = new FAccountWidget(account(name, type), this);
    } else if (type == "owncloud" || type == "runnerid") {
        accountWidget = new OAccountWidget(account(name, type), this);
    }

    m_layout->addWidget(accountWidget);

    QListWidgetItem *newItem = createQListWidgetItem(name, name, type, accountWidget);
    m_ui->accList->addItem(newItem);

    int row = m_ui->accList->row(m_newAccountItem);

    m_ui->accList->takeItem(row);
    m_ui->accList->insertItem(row, newItem);
    m_ui->accList->addItem(m_newAccountItem);

    m_ui->accList->setCurrentItem(newItem);

    KConfigGroup group = account(name, type);

    if (type == "google") {
        createGoogleAccount(group, qobject_cast<AccountWidget *>(accountWidget));
    } else if(type == "facebook") {
        createFacebookAccount(group, qobject_cast<FAccountWidget *>(accountWidget));
    } else if (type == "owncloud" || type == "runnerid") {
        createOwncloudAccount(group, qobject_cast<OAccountWidget *>(accountWidget));
    }
}

QListWidgetItem* WebAccounts::createQListWidgetItem(const QString& name, const QString& title, const QString& type, QWidget *widget)
{
    QListWidgetItem *newItem = new QListWidgetItem();
    newItem->setIcon(QIcon::fromTheme(iconForType(type)));
    newItem->setText(name);
    newItem->setData(Qt::UserRole, QVariant(title));
    newItem->setData(Qt::UserRole + 1, QVariant::fromValue<QWidget *>(widget));
    newItem->setData(Qt::UserRole + 2, QVariant(type));

    return newItem;
}

KConfigGroup WebAccounts::accounts()
{
    return KSharedConfig::openConfig("webaccounts")->group("accounts");
}

KConfigGroup WebAccounts::account(const QString& accName, const QString &type)
{
    return accounts().group(type).group(accName);
}

QString WebAccounts::iconForType(const QString& type)
{
    if (type == "google") {
        return "gmail";
    }

    if (type == "facebook") {
        return "facebookresource";
    }

    if (type == "new") {
        return "applications-education-miscellaneous";
    }

    if (type == "runnerid") {
        return "netrunnerid";
    }

    return type;
}

void WebAccounts::createGoogleAccount(KConfigGroup group, AccountWidget* accountWidget)
{
    KConfigGroup services = group.group("services");

    if (services.readEntry("Contact", 0) == 2) {
        CreateContact *create = new CreateContact(group, this);
        connect(create, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        create->start();
    }

    if (services.readEntry("Calendar", 0) == 2) {
        CreateCalendar *createCalendar = new CreateCalendar(group, this);
        connect(createCalendar, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        if (services.readEntry("Tasks", 0) == 2) {
            CreateTask *createTasks = new CreateTask(group, this);
            connect(createCalendar, SIGNAL(result(KJob*)), createTasks, SLOT(startByCalendar()));
            connect(createTasks, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        }
        createCalendar->start();
    } else if(services.readEntry("Tasks", 0) == 2) {
        CreateTask *createTasks = new CreateTask(group, this);
        connect(createTasks, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        createTasks->start();
    }

    if (services.readEntry("EMail", 0) == 2) {
        CreateMail *createMail = new CreateMail(group, this);
        connect(createMail, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        createMail->start();
    }

    if (services.readEntry("Chat", 0) == 2) {
        CreateChat *createChat = new CreateChat(group, this);
        connect(createChat, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        createChat->start();
    }
}

void WebAccounts::removeGoogleAccount(KConfigGroup group)
{
    KConfigGroup services = group.group("services");
    if (services.readEntry("EMail", 0) == 1) {
        RemoveEmail *removeEmail = new RemoveEmail(group, this);
        connect(removeEmail, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeEmail->start();
    }

    if (services.readEntry("Contact", 0) == 1) {
        RemoveAkonadiResource *removeContact = new RemoveAkonadiResource("contactResource", "Contact", group, this);
        connect(removeContact, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeContact->start();
    }

    if (services.readEntry("Calendar", 0) == 1) {
        RemoveCalendar *removeCalendar = new RemoveCalendar(group, this);
        connect(removeCalendar, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeCalendar->start();
    }

     if (services.readEntry("Tasks", 0) == 1) {
        RemoveTask *removeCalendar = new RemoveTask(group, this);
        connect(removeCalendar, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeCalendar->start();
     }

    if (services.readEntry("Chat", 0) == 1) {
        RemoveChat *removeChat = new RemoveChat(group, this);
        connect(removeChat, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeChat->start();
    }
}

void WebAccounts::createFacebookAccount(KConfigGroup group, FAccountWidget* accountWidget)
{
    KConfigGroup services = group.group("services");

    if (services.readEntry("Chat", 0) == 2) {
        FCreateChat *create = new FCreateChat(group, this);
        connect(create, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        create->start();
    }

    if (services.readEntry("PIM", 0) == 2) {
        FCreatePIM *create = new FCreatePIM(group, this);
        connect(create, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        create->start();
    }
}

void WebAccounts::removeFacebookACcount(KConfigGroup group)
{
    KConfigGroup services = group.group("services");
    if (services.readEntry("Chat", 0) == 1) {
        RemoveChat *removeChat = new RemoveChat(group, this);
        connect(removeChat, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeChat->start();
    }

    if (services.readEntry("PIM", 0) == 1) {
        RemoveAkonadiResource *removeContact = new RemoveAkonadiResource("facebookResource", "PIM", group, this);
        connect(removeContact, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeContact->start();
    }
}

void WebAccounts::createOwncloudAccount(KConfigGroup group, OAccountWidget* accountWidget)
{
    KConfigGroup services = group.group("services");
    if (services.readEntry("File", 0) == 2) {
        OCreateFile *create = new OCreateFile(group, this);
        connect(create, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        create->start();
    }
    if (services.readEntry("Contact", 0) == 2) {
        OCreateContact *contact = new OCreateContact(group, this);
        connect(contact, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
        connect(contact, SIGNAL(finished(KJob*)), this, SLOT(createOCalendar(KJob*)));
        contact->start();
    }
}

void WebAccounts::removeOwncloudAccount(KConfigGroup group)
{
    KConfigGroup services = group.group("services");
    if (services.readEntry("Calendar", 0) == 1) {
        ORemoveCalendar *removeCalendar = new ORemoveCalendar(group, this);
        connect(removeCalendar, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeCalendar->start();
    }

    if (services.readEntry("Contact", 0) == 1) {
        ORemoveContact *removeContact = new ORemoveContact(group, this);
        connect(removeContact, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeContact->start();
    }

    if (services.readEntry("File", 0) == 1) {
        ORemoveFile *removeFile = new ORemoveFile(group, this);
        connect(removeFile, SIGNAL(finished(KJob*)), this, SLOT(serviceRemoved(KJob*)));
        removeFile->start();
    }
}


void WebAccounts::createOCalendar(KJob* job)
{
    kDebug() << "Creating oCalendar";
    KConfigGroup group  = qobject_cast< OCreateContact* >(job)->config();
    OCreateCalendar *calendar = new OCreateCalendar(group, this);
//     connect(calendar, SIGNAL(finished(KJob*)), accountWidget, SLOT(updateAll()));
    calendar->start();
}


#include "webaccounts.moc"
