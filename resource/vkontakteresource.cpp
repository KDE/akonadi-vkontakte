/* Copyright 2010, 2011 Thomas McGuire <mcguire@kde.org>
   Copyright 2011 Roeland Jago Douma <unix@rullzer.com>
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
#include "vkontakteresource.h"
#include <config.h>
#include "settings.h"
#include "settingsdialog.h"

#include <libkvkontakte/friendlistjob.h>
// #include <libkvkontakte/alleventslistjob.h>
// #include <libkvkontakte/eventjob.h>
#include <libkvkontakte/allnoteslistjob.h>
#include <libkvkontakte/notejob.h>
#include <libkvkontakte/noteaddjob.h>
#include <libkvkontakte/vkontaktejobs.h>

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <akonadi/changerecorder.h>
#include <KMime/Message>
#include <libkvkontakte/userinfofulljob.h>
#include <libkvkontakte/allmessageslistjob.h>

using namespace Akonadi;

static const char * friendsRID = "friends";
static const char * messagesRID = "private_messages";
//static const char * eventsRID = "events";
//static const char * eventMimeType = "application/x-vnd.akonadi.calendar.event";
static const char * notesRID = "notes";

VkontakteResource::VkontakteResource( const QString &id )
    : ResourceBase( id )
{
    //AttributeFactory::registerAttribute<TimeStampAttribute>();
    setNeedsNetwork( true );
#if KDEPIMLIBS_IS_VERSION( 4, 6, 41 )
    setAutomaticProgressReporting( false );
#endif
    setObjectName( QLatin1String( "VkontakteResource" ) );
    resetState();
    Settings::self()->setResourceId( identifier() );

    connect( this, SIGNAL(abortRequested()),
             this, SLOT(slotAbortRequested()) );
    connect( this, SIGNAL(reloadConfiguration()), SLOT(configurationChanged()) );

    changeRecorder()->fetchCollection( true );
    changeRecorder()->itemFetchScope().fetchFullPayload( true );
}

VkontakteResource::~VkontakteResource()
{
    Settings::self()->writeConfig();
}

void VkontakteResource::configurationChanged()
{
    Settings::self()->writeConfig();
}

void VkontakteResource::aboutToQuit()
{
    slotAbortRequested();
}

void VkontakteResource::abort()
{
    resetState();
    cancelTask();
}

void VkontakteResource::abortWithError(const QString& errorMessage, bool authFailure)
{
    resetState();
    cancelTask(errorMessage);

    // This doesn't work, why?
    if (authFailure) {
        emit status(Broken, i18n("Unable to login to Vkontakte, authentication failure."));
    }
}

void VkontakteResource::resetState()
{
    m_idle = true;
    m_numFriends = -1;
    m_numPhotosFetched = 0;
    m_currentJobs.clear();
    m_existingFriends.clear();
    m_newOrChangedFriends.clear();
    m_pendingFriends.clear();
}

void VkontakteResource::slotAbortRequested()
{
    if (!m_idle) {
        foreach(const QPointer<KJob> &job, m_currentJobs) {
            kDebug() << "Killing current job:" << job;
            job->kill(KJob::Quietly);
        }
        abort();
    }
}

void VkontakteResource::configure( WId windowId )
{
    const QPointer<SettingsDialog> settingsDialog( new SettingsDialog( this, windowId ) );
    if ( settingsDialog->exec() == QDialog::Accepted ) {
        emit configurationDialogAccepted();
    }
    else {
        emit configurationDialogRejected();
    }

    delete settingsDialog;
}

void VkontakteResource::retrieveItems( const Akonadi::Collection &collection )
{
    Q_ASSERT(m_idle);

    if ( collection.remoteId() == friendsRID ) {
        m_idle = false;
        emit status( Running, i18n( "Preparing sync of friends list." ) );
        emit percent( 0 );
        // Fetching from Akonadi database
        ItemFetchJob * const fetchJob = new ItemFetchJob( collection );
        //fetchJob->fetchScope().fetchAttribute<TimeStampAttribute>();
        fetchJob->fetchScope().fetchFullPayload( false );
        m_currentJobs << fetchJob;
        connect( fetchJob, SIGNAL(result(KJob*)), this, SLOT(initialItemFetchFinished(KJob*)) );
    }
//   else if ( collection.remoteId() == eventsRID ) {
//     mIdle = false;
//     emit status( Running, i18n( "Preparing sync of events list." ) );
//     emit percent( 0 );
//     AllEventsListJob * const listJob = new AllEventsListJob( Settings::self()->accessToken() );
//     listJob->setLowerLimit(KDateTime::fromString( Settings::self()->lowerLimit(), "%Y-%m-%d" ));
//     mCurrentJobs << listJob;
//     connect( listJob, SIGNAL(result(KJob*)), this, SLOT(eventListFetched(KJob*)) );
//     listJob->start();
  //} else

    else if ( collection.remoteId() == notesRID )
    {
        m_idle = false;
        emit status( Running, i18n( "Preparing sync of notes list." ) );
        emit percent( 0 );
        Vkontakte::AllNotesListJob * const notesJob = new Vkontakte::AllNotesListJob(Settings::self()->accessToken(), 0);
//        notesJob->setLowerLimit(KDateTime::fromString( Settings::self()->lowerLimit(), "%Y-%m-%d" ));
        m_currentJobs << notesJob;
        connect( notesJob, SIGNAL(result(KJob*)), this, SLOT(noteListFetched(KJob*)) );
        notesJob->start();
    }
    else if ( collection.remoteId() == messagesRID )
    {
        m_idle = false;
        emit status( Running, i18n( "Preparing sync of messages list." ) );
        emit percent( 0 );
        Vkontakte::AllMessagesListJob * const messagesJob = new Vkontakte::AllMessagesListJob(Settings::self()->accessToken());
        m_currentJobs << messagesJob;
        connect(messagesJob, SIGNAL(result(KJob*)), this, SLOT(messageListFetched(KJob*)));
        messagesJob->start();
    }
    else
    {
        // This can not happen
        Q_ASSERT(!"Unknown Collection");
        cancelTask();
    }
}

bool VkontakteResource::retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts)
{
    Q_UNUSED( parts );

    kDebug() << item.mimeType();

    // may be we should distinguish them by collection RID?
    if (item.mimeType() == "text/directory") {
        // TODO: Is this ever called??
        m_idle = false;
        Vkontakte::UserInfoFullJob * const friendJob = new Vkontakte::UserInfoFullJob( Settings::self()->accessToken(), item.remoteId().toInt() );
        m_currentJobs << friendJob;
        friendJob->setProperty( "Item", QVariant::fromValue( item ) );
        connect( friendJob, SIGNAL(result(KJob*)), this, SLOT(friendJobFinished(KJob*)) );
        friendJob->start();
    }
    else if (item.mimeType() == "text/x-vnd.akonadi.note") {
        m_idle = false;
        Vkontakte::NoteJob * const noteJob = new Vkontakte::NoteJob(Settings::self()->accessToken(), item.remoteId().toInt());
        m_currentJobs << noteJob;
        noteJob->setProperty( "Item", QVariant::fromValue( item ) );
        connect( noteJob, SIGNAL(result(KJob*)), this, SLOT(noteJobFinished(KJob*)) );
        noteJob->start();
    }
    // this won't be called
//     else if (item.mimeType() == KMime::Message::mimeType()) {
//         m_idle = false;
//         MessageJob * const messageJob = new MessageJob(Settings::self()->accessToken(), item.remoteId());
//         m_currentJobs << messageJob;
//         messageJob->setProperty( "Item", QVariant::fromValue( item ) );
//         connect( messageJob, SIGNAL(result(KJob*)), this, SLOT(messageJobFinished(KJob*)) );
//         messageJob->start();
//     }
    return true;
}

void VkontakteResource::retrieveCollections()
{
    Collection friends;
    friends.setRemoteId( friendsRID );
    friends.setName( i18n( "Vkontakte Friends" ) );
    friends.setParentCollection( Akonadi::Collection::root() );
    friends.setContentMimeTypes( QStringList() << "text/directory" );
    friends.setRights( Collection::ReadOnly );
    EntityDisplayAttribute * const friendsDisplayAttribute = new EntityDisplayAttribute();
    friendsDisplayAttribute->setIconName( "vkontakteresource" );
    friends.addAttribute( friendsDisplayAttribute );

//   Collection events;
//   events.setRemoteId( eventsRID );
//   events.setName( i18n( "Events" ) );
//   events.setParentCollection( Akonadi::Collection::root() );
//   events.setContentMimeTypes( QStringList() << "text/calendar" << eventMimeType );
//   events.setRights( Collection::ReadOnly );
//   EntityDisplayAttribute * const evendDisplayAttribute = new EntityDisplayAttribute();
//   evendDisplayAttribute->setIconName( "vkontakteresource" );
//   events.addAttribute( evendDisplayAttribute );

    Collection notes;
    notes.setRemoteId( notesRID );
    notes.setName( i18n( "Vkontakte Notes" ) );
    notes.setParentCollection( Akonadi::Collection::root() );
    notes.setContentMimeTypes( QStringList() << "text/x-vnd.akonadi.note"  );
    notes.setRights(Collection::ReadOnly);
    EntityDisplayAttribute * const notesDisplayAttribute = new EntityDisplayAttribute();
    notesDisplayAttribute->setIconName( "vkontakteresource" );
    notes.addAttribute( notesDisplayAttribute );

    Collection messages;
    messages.setRemoteId(messagesRID);
    messages.setName( i18n( "Vkontakte Private Messages" ) );
    messages.setParentCollection( Akonadi::Collection::root() );
    messages.setContentMimeTypes( QStringList() << KMime::Message::mimeType() << Collection::mimeType() );
    messages.setRights(Collection::ReadOnly);
    EntityDisplayAttribute * const messagesDisplayAttribute = new EntityDisplayAttribute();
    messagesDisplayAttribute->setIconName( "vkontakteresource" );
    messages.addAttribute( messagesDisplayAttribute );

    collectionsRetrieved( Collection::List() << friends /*<< events*/ << notes << messages );
}

/*void VkontakteResource::itemRemoved(const Akonadi::Item &item)
{
  if (item.mimeType() == "text/x-vnd.akonadi.note") {
    mIdle = false;
    VkontakteDeleteJob * const deleteJob = new VkontakteDeleteJob( item.remoteId(),
                                               Settings::self()->accessToken() );
    mCurrentJobs << deleteJob;
    deleteJob->setProperty( "Item", QVariant::fromValue( item ) );
    connect( deleteJob, SIGNAL(result(KJob*)), this, SLOT(deleteJobFinished(KJob*)) );
    deleteJob->start();
  } else {
    // Shouldn't happen, all other items are read-only
    Q_ASSERT(!"Unable to delete item, not ours.");
    cancelTask();
  }
}*/

// void VkontakteResource::deleteJobFinished(KJob *job)
// {
//   Q_ASSERT(!mIdle);
//   Q_ASSERT( mCurrentJobs.indexOf(job) != -1 );
//   mCurrentJobs.removeAll(job);
//   if ( job->error() ) {
//     abortWithError( i18n( "Unable to delete note from server: %1", job->errorText() ) );
//   } else {
//     const Item item = job->property( "Item" ).value<Item>();
//     changeCommitted( item );
//     resetState();
//   }
// }

// void VkontakteResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
// {
//   if (collection.remoteId() == notesRID) {
//     if (item.hasPayload<KMime::Message::Ptr>()) {
//       const KMime::Message::Ptr note = item.payload<KMime::Message::Ptr>();
//       const QString subject = note->subject()->asUnicodeString();
//       const QString message = note->body();
//
//       mIdle = false;
//       NoteAddJob * const addJob = new NoteAddJob( subject, message, Settings::self()->accessToken() );
//       mCurrentJobs << addJob;
//       addJob->setProperty( "Item", QVariant::fromValue( item ) );
//       connect( addJob, SIGNAL(result(KJob *)), this, SLOT(noteAddJobFinished(KJob *)) );
//       addJob->start();
//     } else {
//       Q_ASSERT(!"Note has wrong mimetype.");
//       cancelTask();
//     }
//   } else {
//     Q_ASSERT(!"Can not add this type of item!");
//     cancelTask();
//   }
// }

// static
KABC::Addressee VkontakteResource::toPimAddressee(const Vkontakte::UserInfo &o)
{
    KABC::Addressee addressee;
    addressee.setGivenName( o.firstName() );
    addressee.setUid( QString::number(o.uid()) );
    addressee.setFamilyName( o.lastName() );
    //addressee.setFormattedName( name() );
    addressee.setUrl( o.profileUrl() );
    addressee.setBirthday( QDateTime( o.birthday() ) );
    //addressee.setOrganization(mCompany);
    if (o.timezone() != Vkontakte::UserInfo::INVALID_TIMEZONE) {
        addressee.setTimeZone(KABC::TimeZone(o.timezone()));
    }
    //addressee.insertCustom("KADDRESSBOOK", "X-Profession", mProfession);
    //addressee.insertCustom("KADDRESSBOOK", "X-SpousesName", mPartner);
    if ( !o.countryString().isEmpty() || !o.cityString().isEmpty() ) {
        KABC::Address address(KABC::Address::Home);
        address.setRegion(o.countryString());
        address.setLocality(o.cityString());
        addressee.insertAddress(address);
    }

    if (!o.homePhone().isEmpty()) {
        KABC::PhoneNumber number;
        number.setNumber(o.homePhone());
        number.setType(KABC::PhoneNumber::Home);
        addressee.insertPhoneNumber(number);
    }
    if (!o.mobilePhone().isEmpty()) {
        KABC::PhoneNumber number;
        number.setNumber(o.mobilePhone());
        number.setType(KABC::PhoneNumber::Cell);
        addressee.insertPhoneNumber(number);
    }

    return addressee;
}

// static
KMime::Message::Ptr VkontakteResource::toPimNote(const Vkontakte::NoteInfo &o)
{
    KMime::Message * const note = new KMime::Message();

    note->date()->fromUnicodeString( o.date().toString(KDateTime::RFCDateDay), "utf-8" );
    note->contentType()->setMimeType("text/html");
    note->contentType()->setCharset("utf-8");

    note->subject()->fromUnicodeString( o.title(), "utf-8" );
    note->from()->fromUnicodeString( "you@vkontakte", "utf-8" );
    note->contentTransferEncoding()->setEncoding(KMime::Headers::CEquPr);

    QString m = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n";
    m += "<html><head></head><body>\n";
    m += o.text();
    m += "</body>";
    note->fromUnicodeString(m);

    note->assemble();

    return KMime::Message::Ptr(note);
}

KMime::Message::Ptr VkontakteResource::toPimMessage(
    const Vkontakte::MessageInfo &o,
    QString userAddress, QString ownAddress,
    QString messageId, QString inReplyTo)
{
    if (userAddress.isEmpty())
        userAddress = QString("<unknown@vkontakte>");
    if (ownAddress.isEmpty())
        ownAddress = QString("<you@vkontakte>");

    // http://api.kde.org/4.x-api/kdepimlibs-apidocs/kmime/html/classKMime_1_1Message.html#a5614aa32a42b034f5290d6d7a56cc433
    KMime::Message *mail = new KMime::Message();

    mail->from()->fromUnicodeString( o.out() ? ownAddress : userAddress, "utf-8" );
    mail->to()->fromUnicodeString( !o.out() ? ownAddress : userAddress, "utf-8" );
    mail->date()->setDateTime( o.date() );
    mail->subject()->fromUnicodeString( o.title(), "utf-8" );

    // http://api.kde.org/4.x-api/kdepimlibs-apidocs/kmime/html/index.html
    // This snippet was written by Thomas McGuire
    mail->contentType()->setMimeType( "text/plain" );
    mail->contentType()->setCharset("utf-8");
    mail->fromUnicodeString( o.body() );
    mail->contentTransferEncoding()->setEncoding(KMime::Headers::CEbase64);

    if (!messageId.isEmpty())
        mail->messageID()->from7BitString(messageId.toAscii());
    if (!inReplyTo.isEmpty()) {
        mail->inReplyTo()->from7BitString(inReplyTo.toAscii());
        //mail->references()->from7BitString(inReplyTo.toAscii());
    }

    mail->assemble();

    return KMime::Message::Ptr(mail);
}

AKONADI_RESOURCE_MAIN( VkontakteResource )
