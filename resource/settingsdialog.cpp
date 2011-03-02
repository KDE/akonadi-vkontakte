/* Copyright 2010 Thomas McGuire <mcguire@kde.org>

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
#include "facebookresource.h"
#include "settings.h"

#include <libkfacebook/authenticationdialog.h>
#include <libkfacebook/userinfojob.h>
#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KWindowSystem>

using namespace Akonadi;

SettingsDialog::SettingsDialog( FacebookResource *parentResource, WId parentWindow )
  : KDialog(),
    mParentResource( parentResource ),
    mTriggerSync( false )
{
  KWindowSystem::setMainWindow( this, parentWindow );
  setButtons( Ok|Cancel|User1 );
  setButtonText(User1, i18n("About"));
  setButtonIcon(User1, KIcon("help-about"));
  setWindowIcon( KIcon( "facebookresource" ) );
  setWindowTitle( i18n("Facebook Settings") );

  setupWidgets();
  loadSettings();
}

SettingsDialog::~SettingsDialog()
{
  if ( mTriggerSync ) {
    mParentResource->synchronize();
  }
}

void SettingsDialog::setupWidgets()
{
  QWidget * const page = new QWidget( this );
  setupUi( page );
  setMainWidget( page );
  updateAuthenticationWidgets();
  updateUserName();
  connect( resetButton, SIGNAL(clicked(bool)), this, SLOT(resetAuthentication()) );
  connect( authenticateButton, SIGNAL(clicked(bool)), this, SLOT(showAuthenticationDialog()) );
}

void SettingsDialog::showAuthenticationDialog()
{
  QStringList permissions;
  permissions << "offline_access"
              << "friends_birthday"
              << "friends_website"
              << "friends_location"
              << "friends_work_history"
              << "friends_relationships"
              << "user_events"
              << "user_notes"
              << "create_note";
  AuthenticationDialog * const authDialog = new AuthenticationDialog( this );
  authDialog->setAppId( Settings::self()->appID() );
  authDialog->setPermissions( permissions );
  connect( authDialog, SIGNAL(authenticated(QString)),
           this, SLOT(authenticationDone(QString)) );
  connect( authDialog, SIGNAL(canceled()),
           this, SLOT(authenticationCanceled()) );
  authenticateButton->setEnabled( false );
  authDialog->start();
}

void SettingsDialog::authenticationCanceled()
{
  authenticateButton->setEnabled( true );
}

void SettingsDialog::authenticationDone(const QString& accessToken)
{
  if ( Settings::self()->accessToken() != accessToken && !accessToken.isEmpty() ) {
    mTriggerSync = true;
  }
  Settings::self()->setAccessToken( accessToken );
  updateAuthenticationWidgets();
  updateUserName();
}

void SettingsDialog::updateAuthenticationWidgets()
{
  if ( Settings::self()->accessToken().isEmpty() ) {
    authenticationStack->setCurrentIndex( 0 );
  } else {
    authenticationStack->setCurrentIndex( 1 );
    if ( Settings::self()->userName().isEmpty() ) {
      authenticationLabel->setText( i18n( "Authenticated." ) );
    } else {
      authenticationLabel->setText( i18n( "Authenticated as <b>%1</b>.", Settings::self()->userName() ) );
    }
  }
}

void SettingsDialog::resetAuthentication()
{
  Settings::self()->setAccessToken( QString() );
  Settings::self()->setUserName( QString() );
  updateAuthenticationWidgets();
}

void SettingsDialog::updateUserName()
{
  if ( Settings::self()->userName().isEmpty() && ! Settings::self()->accessToken().isEmpty() ) {
    UserInfoJob * const job = new UserInfoJob( Settings::self()->accessToken() );
    connect( job, SIGNAL(result(KJob*)), this, SLOT(userInfoJobDone(KJob*)) );
    job->start();
  }
}

void SettingsDialog::userInfoJobDone( KJob* job )
{
  UserInfoJob * const userInfoJob = dynamic_cast<UserInfoJob*>( job );
  Q_ASSERT( userInfoJob );
  if ( !userInfoJob->error() ) {
    Settings::self()->setUserName( userInfoJob->userInfo()->name() );
    updateAuthenticationWidgets();
  } else {
    kWarning() << "Can't get user info: " << userInfoJob->errorText();
  }
}

void SettingsDialog::loadSettings()
{
  if ( mParentResource->name() == mParentResource->identifier() )
    mParentResource->setName( i18n( "Facebook" ) );

  nameEdit->setText( mParentResource->name() );
  nameEdit->setFocus();
}

void SettingsDialog::saveSettings()
{
  mParentResource->setName( nameEdit->text() );
  Settings::self()->writeConfig();
}

void SettingsDialog::slotButtonClicked( int button )
{
  switch( button ) {
    case Ok:
      saveSettings();
      accept();
      break;
    case Cancel:
      reject();
      return;
    case User1: {
      KAboutData aboutData( QByteArray( "akonadi_facebook_resource" ),
                            QByteArray(),
                            ki18n("Akonadi Facebook Resource"),
                            QByteArray( RESOURCE_VERSION ),
                            ki18n( "Makes your friends, events, notes and messages on Facebook available in KDE via Akonadi." ),
                            KAboutData::License_GPL_V2,
                            ki18n( "Copyright (C) 2010,2011 Thomas McGuire <mcguire@kde.org>" ) );
      aboutData.addAuthor( ki18n( "Thomas McGuire" ), ki18n( "Maintainer" ), "mcguire@kde.org" );
      aboutData.addAuthor( ki18n( "Till Adam" ), ki18n( "MacOS Support" ), "adam@kde.org" );
      aboutData.addAuthor( ki18n( "Roeland Jago Douma" ), ki18n( "Notes Implementation" ), "unix@rullzer.com" );
      aboutData.setProgramIconName("facebookresource");
      aboutData.setTranslator( ki18nc("NAME OF TRANSLATORS", "Your names"),
                            ki18nc("EMAIL OF TRANSLATORS", "Your emails"));
      KAboutApplicationDialog *dialog = new KAboutApplicationDialog(&aboutData, this);
      dialog->setAttribute( Qt::WA_DeleteOnClose, true );
      dialog->show();
      break;
    }
  }
}

#include "settingsdialog.moc"
