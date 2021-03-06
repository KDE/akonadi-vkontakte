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
#ifndef VKONTAKTERESOURCE_H
#define VKONTAKTERESOURCE_H

#include <QtCore/QPointer>
#include <QtCore/QMutex>
#include <Akonadi/ResourceBase>
#include <KABC/Addressee>
#include <KMime/Message>
#include <libkvkontakte/userinfo.h>
#include <libkvkontakte/messageinfo.h>
#include <libkvkontakte/noteinfo.h>

class VkontakteResource : public Akonadi::ResourceBase,
    public Akonadi::AgentBase::Observer
{
    Q_OBJECT

public:
    VkontakteResource(const QString &id);
    ~VkontakteResource();

    using ResourceBase::synchronize;

public Q_SLOTS:
    virtual void configure(WId windowId);

protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems(const Akonadi::Collection &col);
    bool retrieveItem(const Akonadi::Item &item, const QSet<QByteArray> &parts);

//    void itemRemoved( const Akonadi::Item &item);
//    void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );

protected:

    virtual void aboutToQuit();

private Q_SLOTS:

    void slotAbortRequested();
    void configurationChanged();

    // Friends
    void friendListJobFinished(KJob *kjob);
    void friendJobFinished(KJob *kjob);
    void photoJobFinished(KJob *kjob);
    void detailedFriendListJobFinished(KJob *kjob);
    void initialItemFetchFinished(KJob *kjob);

    // Events
    //void eventListFetched(KJob *kjob);
    //void detailedEventListJobFinished(KJob *kjob);
    
    // Notes
    void noteListFetched(KJob *kjob);
    void noteJobFinished(KJob *kjob);
    //void noteAddJobFinished(KJob *kjob);
    //void deleteJobFinished(KJob *kjob);

    // Messages
    void messageListFetched(KJob *kjob);
    void messageListUsersFetched(KJob *kjob);
    void messageDiscussionsFetched(KJob *kjob);
//     void messageJobFinished(KJob *kjob);
    //void noteAddJobFinished(KJob *kjob);
    //void deleteJobFinished(KJob *kjob);

private:
    void fetchPhotos();
    void resetState();
    void abortWithError(const QString &errorMessage, bool authFailure = false);
    void abort();

    void fetchNewOrChangedFriends();
    void finishFriendFetching();
    void finishEventsFetching();
    void finishNotesFetching();
    void finishMessagesFetching();

    /**
    * @brief Created a KABC::Addressee for all the information we have about this person.
    *
    * @return A KABC::Addressee of this person.
    */
    static KABC::Addressee toPimAddressee(const Vkontakte::UserInfo &o);

    /**
     * Generates a KMime::Message from this note and return a
     * KMime::Message::Ptr to it.
     */
    static KMime::Message::Ptr toPimNote(const Vkontakte::NoteInfo &o);

    KMime::Message::Ptr toPimMessage(const Vkontakte::MessageInfo &o,
                                     QString userAddress = QString(),
                                     QString ownAddress = QString(),
                                     QString messageId = QString(),
                                     QString inReplyTo = QString());


    // Friends that are already stored on the Akonadi server
    QMap<int, KDateTime> m_existingFriends;

    // Pending new/changed friends we still need to download
    QList<Vkontakte::UserInfoPtr> m_pendingFriends;

    QList<Vkontakte::UserInfoPtr> m_newOrChangedFriends;

    // Total number of new & changed friends
    int m_numFriends;
    int m_numPhotosFetched;

    // For messages retrieval
    QList<Vkontakte::MessageInfoPtr> m_allMessages;
    QMap<int, Vkontakte::UserInfoPtr> m_messagesUsersMap;

    bool m_idle;
    QList< QPointer<KJob> > m_currentJobs;
};

#endif
