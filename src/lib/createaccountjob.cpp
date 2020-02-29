/*************************************************************************************
 *  Copyright (C) 2013 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
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

#include "createaccountjob.h"
#include "kaccountsuiplugin.h"
#include "core.h"
#include "uipluginsmanager.h"

#include <QDebug>
#include <QPluginLoader>
#include <QDir>

#include <Accounts/Manager>
#include <Accounts/AccountService>

#include <SignOn/Identity>
#include <SignOn/AuthSession>

#include <KLocalizedString>

CreateAccountJob::CreateAccountJob(QObject* parent)
 : CreateAccountJob(QString(), parent)
{

}

CreateAccountJob::CreateAccountJob(const QString &providerName, QObject* parent)
 : KJob(parent)
 , m_providerName(providerName)
 , m_manager(new Accounts::Manager(this))
 , m_account(nullptr)
 , m_accInfo(nullptr)
 , m_identity(nullptr)
 , m_done(false)
{

}

void CreateAccountJob::start()
{
    qDebug() << m_providerName;
    QMetaObject::invokeMethod(this, "processSession");
}

void CreateAccountJob::processSession()
{
    m_account = m_manager->createAccount(m_providerName);
    Accounts::Service service;
    if (m_account->services().size() == 1) {
        service = m_account->services().at(0);
    }
    m_accInfo = new Accounts::AccountService(m_account, service, this);

    const QString pluginName = m_account->provider().pluginName();
    qDebug() << "Looking for plugin" << pluginName;
    if (!pluginName.isEmpty()) {
        loadPluginAndShowDialog(pluginName);
    } else {
        SignOn::IdentityInfo info;
        info.setCaption(m_providerName);
        info.setAccessControlList({QStringLiteral("*")});
        info.setType(SignOn::IdentityInfo::Application);
        info.setStoreSecret(true);

        m_identity = SignOn::Identity::newIdentity(info, this);
        m_identity->storeCredentials();

        connect(m_identity, &SignOn::Identity::info, this, &CreateAccountJob::info);
        connect(m_identity, &SignOn::Identity::error, [=](const SignOn::Error &err) {
            qDebug() << "Error storing identity:" << err.message();
        });

        QVariantMap data = m_accInfo->authData().parameters();
        data.insert(QStringLiteral("Embedded"), false);

        SignOn::SessionData sessionData(data);
        SignOn::AuthSessionP session = m_identity->createSession(m_accInfo->authData().method());
        qDebug() << "Starting auth session with" << m_accInfo->authData().method();
        connect(session, &SignOn::AuthSession::error, this, &CreateAccountJob::sessionError);
        connect(session, &SignOn::AuthSession::response, this, &CreateAccountJob::sessionResponse);

        session->process(sessionData, m_accInfo->authData().mechanism());
    }
}

void CreateAccountJob::loadPluginAndShowDialog(const QString &pluginName)
{
    KAccountsUiPlugin *ui = KAccounts::UiPluginsManager::pluginForName(pluginName);

    if (!ui) {
        qDebug() << "Plugin could not be loaded";
        pluginError(i18nc("The %1 is for plugin name, eg. Could not load UI plugin", "Could not load %1 plugin, please check your installation", pluginName));
        return;
    }

    connect(ui, &KAccountsUiPlugin::success, this, &CreateAccountJob::pluginFinished, Qt::UniqueConnection);
    connect(ui, &KAccountsUiPlugin::error, this, &CreateAccountJob::pluginError, Qt::UniqueConnection);

    ui->setProviderName(m_providerName);
    ui->init(KAccountsUiPlugin::NewAccountDialog);
}

void CreateAccountJob::pluginFinished(const QString &screenName, const QString &secret, const QVariantMap &data)
{
    // Set up the new identity
    SignOn::IdentityInfo info;
    info.setStoreSecret(true);
    info.setUserName(screenName);
    info.setSecret(secret, true);
    info.setCaption(m_providerName);
    info.setAccessControlList(QStringList(QLatin1String("*")));
    info.setType(SignOn::IdentityInfo::Application);

    const auto keys = data.keys();
    for (const QString &key : keys) {
        // If a key with __service/ prefix exists and its value is false,
        // add it to m_disabledServices which will later be used for disabling
        // the services contained in that list
        if (key.startsWith(QLatin1String("__service/")) && !data.value(key).toBool()) {
            m_disabledServices << key.mid(10);
        }
        m_account->setValue(key, data.value(key).toString());
    }

    m_identity = SignOn::Identity::newIdentity(info, this);
    connect(m_identity, &SignOn::Identity::info, this, &CreateAccountJob::info);

    m_done = true;

    connect(m_identity, &SignOn::Identity::credentialsStored, m_identity, &SignOn::Identity::queryInfo);
    m_identity->storeCredentials();
}

void CreateAccountJob::pluginError(const QString &error)
{
    if (error.isEmpty()) {
        setError(-1);
    } else {
        setError(KJob::UserDefinedError);
    }
    setErrorText(error);
    // Delete the dialog
    emitResult();
}

void CreateAccountJob::sessionResponse(const SignOn::SessionData &/*data*/)
{
    qDebug() << "Received session response";

    m_done = true;
    m_identity->queryInfo();
}

void CreateAccountJob::info(const SignOn::IdentityInfo &info)
{
    qDebug() << "Info:";
    qDebug() << "\tId:" << info.id();
    qDebug() << "\tcaption:" << info.caption();
    qDebug() << "\towner:" << info.owner();
    qDebug() << "\tuserName:" << info.userName();

    if (!m_done) {
        return;
    }

    m_account->selectService();

    if (m_account->displayName().isEmpty()) {
        m_account->setDisplayName(info.userName());
    }
    m_account->setValue(QStringLiteral("username"), info.userName());
    m_account->setCredentialsId(info.id());

    Accounts::AuthData authData = m_accInfo->authData();
    m_account->setValue(QStringLiteral("auth/mechanism"), authData.mechanism());
    m_account->setValue(QStringLiteral("auth/method"), authData.method());

    QString base = QStringLiteral("auth/");
    base.append(authData.method());
    base.append(QLatin1Char('/'));
    base.append(authData.mechanism());
    base.append(QLatin1Char('/'));

    QVariantMap data = authData.parameters();
    QMapIterator<QString, QVariant> i(data);
    while (i.hasNext()) {
        i.next();
        m_account->setValue(base + i.key(), i.value());
    }


    const Accounts::ServiceList services = m_account->services();
    for (const Accounts::Service &service : services) {
        m_account->selectService(service);
        m_account->setEnabled(m_disabledServices.contains(service.name()) ? false : true);
    }

    m_account->selectService();
    m_account->setEnabled(true);
    m_account->sync();
    connect(m_account, &Accounts::Account::synced, this, &CreateAccountJob::emitResult);
}

void CreateAccountJob::sessionError(const SignOn::Error &signOnError)
{
    if (error()) {
        // Guard against SignOn sending two error() signals
        return;
    }
    qWarning() << "Error:";
    qWarning() << "\t" << signOnError.message();

    setError(KJob::UserDefinedError);
    setErrorText(i18n("There was an error while trying to process the request: %1", signOnError.message()));
    emitResult();
}

void CreateAccountJob::setProviderName(const QString &name)
{
    if (m_providerName != name) {
        m_providerName = name;
        Q_EMIT providerNameChanged();
    }
}