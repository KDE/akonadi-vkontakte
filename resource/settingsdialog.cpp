/* Copyright 2010 Thomas McGuire <mcguire@kde.org>
   Copyright 2011 Alexander Potashev <aspotashev@gmail.com>

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU Library General Public License as published
   by the Free Software Foundation; either version 2 of the License or
   ( at your option ) version 3 or, at the discretion of KDE e.V.
   ( which shall act as a proxy as in section 14 of the GPLv3 ), any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "settingsdialog.h"
#include "vkontakteresource.h"
#include "settings.h"
#include <config.h>

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KWindowSystem>
#include <libkvkontakte/authenticationdialog.h>
#include <libkvkontakte/userinfojob.h>
#include <libkvkontakte/getvariablejob.h>

using namespace Akonadi;

SettingsDialog::SettingsDialog(VkontakteResource *parentResource, WId parentWindow)
    : KDialog()
    , m_parentResource(parentResource)
    , m_triggerSync(false)
{
    KWindowSystem::setMainWindow(this, parentWindow);
    setButtons(Ok | Cancel | User1);
    setButtonText(User1, i18n("About"));
    setButtonIcon(User1, KIcon("help-about"));
    setWindowIcon(KIcon("vkontakteresource"));
    setWindowTitle(i18n("VKontakte Settings"));

    setupWidgets();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    if (m_triggerSync)
        m_parentResource->synchronize();
}

void SettingsDialog::setupWidgets()
{
    QWidget * const page = new QWidget(this);
    setupUi(page);
    setMainWidget(page);
    updateAuthenticationWidgets();
    updateUserName();
    connect(resetButton, SIGNAL(clicked(bool)), this, SLOT(resetAuthentication()));
    connect(authenticateButton, SIGNAL(clicked(bool)), this, SLOT(showAuthenticationDialog()));
}

void SettingsDialog::showAuthenticationDialog()
{
    QStringList permissions;
    permissions << "notify"
                << "friends"
                << "photos"
                << "audio"
                << "video"
                << "docs"
                << "notes"
                << "pages"
                << "offers"
                << "questions"
                << "wall"
                << "messages"
                << "offline";
    Vkontakte::AuthenticationDialog * const authDialog = new Vkontakte::AuthenticationDialog(this);
    authDialog->setAppId(Settings::self()->appID());
    authDialog->setPermissions(permissions);
    connect(authDialog, SIGNAL(authenticated(QString)),
            this, SLOT(authenticationDone(QString)));
    connect(authDialog, SIGNAL(canceled()),
            this, SLOT(authenticationCanceled()));
    authenticateButton->setEnabled(false);
    authDialog->start();
}

void SettingsDialog::authenticationCanceled()
{
    authenticateButton->setEnabled(true);
}

void SettingsDialog::authenticationDone(const QString &accessToken)
{
    if (Settings::self()->accessToken() != accessToken && !accessToken.isEmpty())
        m_triggerSync = true;

    Settings::self()->setAccessToken(accessToken);
    updateAuthenticationWidgets();
    updateUserName();
}

void SettingsDialog::updateAuthenticationWidgets()
{
    if (Settings::self()->accessToken().isEmpty())
    {
        authenticationStack->setCurrentIndex(0);
    }
    else
    {
        authenticationStack->setCurrentIndex(1);
        if (Settings::self()->userName().isEmpty())
            authenticationLabel->setText(i18n("Authenticated."));
        else
            authenticationLabel->setText(i18n("Authenticated as <b>%1</b>.", Settings::self()->userName()));
    }
}

void SettingsDialog::resetAuthentication()
{
    Settings::self()->setAccessToken(QString());
    Settings::self()->setUserName(QString());
    updateAuthenticationWidgets();
}

void SettingsDialog::updateUserName()
{
    if (Settings::self()->userName().isEmpty() && ! Settings::self()->accessToken().isEmpty())
    {
        Vkontakte::GetVariableJob * const job = new Vkontakte::GetVariableJob(Settings::self()->accessToken(), 1281); // get display name
        connect(job, SIGNAL(result(KJob*)), this, SLOT(userInfoJobDone(KJob*)));
        job->start();
    }
}

void SettingsDialog::userInfoJobDone(KJob *kjob)
{
    Vkontakte::GetVariableJob * const job = dynamic_cast<Vkontakte::GetVariableJob*>(kjob);
    Q_ASSERT(job);
    if (!job->error())
    {
        Settings::self()->setUserName(job->variable().toString());
        updateAuthenticationWidgets();
    }
    else
    {
        kWarning() << "Can't get user info: " << job->errorText();
    }
}

void SettingsDialog::loadSettings()
{
    if (m_parentResource->name() == m_parentResource->identifier())
        m_parentResource->setName(i18n("Vkontakte"));

    nameEdit->setText(m_parentResource->name());
    nameEdit->setFocus();
}

void SettingsDialog::saveSettings()
{
    m_parentResource->setName(nameEdit->text());
    Settings::self()->writeConfig();
}

void SettingsDialog::slotButtonClicked(int button)
{
    switch(button)
    {
    case Ok:
        saveSettings();
        accept();
        break;
    case Cancel:
        reject();
        return;
    case User1: {
        KAboutData aboutData(QByteArray("akonadi_vkontakte_resource"),
                             QByteArray(),
                             ki18n("Akonadi Vkontakte Resource"),
                             QByteArray(RESOURCE_VERSION),
                             ki18n("Makes your friends, notes and messages on Vkontakte available in KDE via Akonadi."),
                             KAboutData::License_GPL_V2,
                             ki18n("(С) 2011 Alexander Potashev"));
        aboutData.addAuthor( ki18n("Alexander Potashev"), ki18n("Maintainer"), "aspotashev@gmail.com");
        aboutData.addCredit( ki18n("Thomas McGuire"), ki18n("Author of akonadi-facebook"), "mcguire@kde.org");
        aboutData.setProgramIconName("vkontakteresource");
        aboutData.setTranslator(ki18nc("NAME OF TRANSLATORS", "Your names"),
                                ki18nc("EMAIL OF TRANSLATORS", "Your emails"));
        KAboutApplicationDialog *dialog = new KAboutApplicationDialog(&aboutData, this);
        dialog->setAttribute(Qt::WA_DeleteOnClose, true);
        dialog->show();
        break;
    }
    }
}

#include "settingsdialog.moc"
