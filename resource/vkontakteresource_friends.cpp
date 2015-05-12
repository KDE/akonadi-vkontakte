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

#include <Akonadi/AttributeFactory>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/ItemFetchJob>
#include <Akonadi/ItemFetchScope>
#include <akonadi/changerecorder.h>
#include <libkvkontakte/friendlistjob.h>
#include <libkvkontakte/userinfofulljob.h>
#include <libkvkontakte/photojob.h>

using namespace Akonadi;

// Called when fetching from Akonadi database is finished
void VkontakteResource::initialItemFetchFinished(KJob *kjob)
{
    Q_ASSERT(!m_idle);
    Q_ASSERT( m_currentJobs.indexOf(kjob) != -1 );
    ItemFetchJob * const job = dynamic_cast<ItemFetchJob*>(kjob);
    Q_ASSERT(job);
    m_currentJobs.removeAll(job);

    if (job->error())
    {
        abortWithError(i18n("Unable to get list of existing friends from the Akonadi server: %1", job->errorString()));
    }
    else
    {
//        foreach( const Item &item, itemFetchJob->items() ) {
//        if ( item.hasAttribute<TimeStampAttribute>() ) // FIXME: why timestamp is needed?
//            m_existingFriends.insert( item.remoteId(), item.attribute<TimeStampAttribute>()->timeStamp() );
//        }

        setItemStreamingEnabled(true);

        // Getting the list of friends of the current user
        Vkontakte::FriendListJob * const friendListJob = new Vkontakte::FriendListJob(Settings::self()->accessToken());
        m_currentJobs << friendListJob;
        connect(friendListJob, SIGNAL(result(KJob*)), this, SLOT(friendListJobFinished(KJob*)));
        emit status(Running, i18n("Retrieving friends list."));
        emit percent(2);
        friendListJob->start();
    }
}

// Called when FriendListJob is finished
void VkontakteResource::friendListJobFinished(KJob *kjob)
{
    Q_ASSERT(!m_idle);
    Q_ASSERT(m_currentJobs.indexOf(kjob) != -1);
    Vkontakte::FriendListJob * const job = dynamic_cast<Vkontakte::FriendListJob*>(kjob);
    Q_ASSERT(job);
    m_currentJobs.removeAll(kjob);

    if (job->error())
    {
        abortWithError(i18n("Unable to get list of friends from VKontakte server: %1", job->errorText()),
                       job->error() == Vkontakte::VkontakteJob::AuthenticationProblem);
    }
    else
    {
        // Figure out which items are new or changed
        foreach(const Vkontakte::UserInfoPtr &user, job->list())
            m_newOrChangedFriends.append(user);

        // Delete items that are in the Akonadi DB but no on FB
        Item::List removedItems;
        foreach (const int friendId, m_existingFriends.keys())
        {
            bool found = false;
            foreach (const Vkontakte::UserInfoPtr &user, job->list())
                if (user->uid() == friendId)
                {
                    found = true;
                    break;
                }

            if (!found)
            {
                kDebug() << friendId << "is no longer your friend :(";
                Item removedItem;
                removedItem.setRemoteId(QString::number(friendId));
                removedItems.append(removedItem);
            }
        }
        itemsRetrievedIncremental(Item::List(), removedItems);

        if (m_newOrChangedFriends.isEmpty())
        {
            finishFriendFetching();
        }
        else
        {
            emit status(Running,
                        i18nc("%1 is the number of friends the currently logged in user has in VKontakte.",
                              "Retrieving friends' details (%1 friends total).").arg(m_newOrChangedFriends.size()));
            emit percent(5);
            fetchNewOrChangedFriends();
        }
    }
}

void VkontakteResource::fetchNewOrChangedFriends()
{
    Vkontakte::QIntList newOrChangedFriendIds;
    foreach (const Vkontakte::UserInfoPtr &user, m_newOrChangedFriends) {
        newOrChangedFriendIds.append(user->uid());
    }
    Vkontakte::UserInfoFullJob * const friendJob = new Vkontakte::UserInfoFullJob(
        Settings::self()->accessToken(), newOrChangedFriendIds);
    m_currentJobs << friendJob;
    connect( friendJob, SIGNAL(result(KJob*)), this, SLOT(detailedFriendListJobFinished(KJob*)) );
    friendJob->start();
}

// UserInfoFullJob for new friends was finished
void VkontakteResource::detailedFriendListJobFinished(KJob *kjob)
{
    Q_ASSERT(!m_idle);
    Q_ASSERT(m_currentJobs.indexOf(kjob) != -1);
    Vkontakte::UserInfoFullJob * const job = dynamic_cast<Vkontakte::UserInfoFullJob*>(kjob);
    Q_ASSERT(job);
    m_currentJobs.removeAll(job);

    if (job->error())
    {
        abortWithError(i18n("Unable to retrieve friends' information from server: %1", job->errorText()));
    }
    else
    {
        m_pendingFriends = job->userInfo();
        m_numFriends = m_pendingFriends.size();
        emit status(Running, i18n("Retrieving friends' photos."));
        emit percent(10);
        fetchPhotos();
    }
}

void VkontakteResource::fetchPhotos()
{
    m_idle = false;
    m_numPhotosFetched = 0;
    foreach (const Vkontakte::UserInfoPtr &f, m_pendingFriends)
    {
        Vkontakte::PhotoJob * const photoJob = new Vkontakte::PhotoJob(KUrl(f->photoMedium()));
        m_currentJobs << photoJob;
        photoJob->setProperty("friend", QVariant::fromValue(f));
        connect(photoJob, SIGNAL(result(KJob*)), this, SLOT(photoJobFinished(KJob*)));
        photoJob->start();
    }
}

void VkontakteResource::finishFriendFetching()
{
    Q_ASSERT(m_currentJobs.size() == 0);

    m_pendingFriends.clear();
    emit percent(100);
    if ( m_numFriends > 0 ) {
        emit status( Idle, i18np( "Updated one friend from the server.",
                                "Updated %1 friends from the server.", m_numFriends ) );
    } else {
        emit status( Idle, i18n( "Finished, no friends needed to be updated." ) );
    }
    resetState();
}

void VkontakteResource::photoJobFinished(KJob *kjob)
{
    Q_ASSERT(!m_idle);
    Q_ASSERT( m_currentJobs.indexOf(kjob) != -1 );
    Vkontakte::PhotoJob * const job = dynamic_cast<Vkontakte::PhotoJob*>(kjob);
    Q_ASSERT(job);
    const Vkontakte::UserInfoPtr user = job->property("friend").value<Vkontakte::UserInfoPtr>();
    m_currentJobs.removeOne(kjob);

    if (job->error())
    {
        abortWithError(i18n("Unable to retrieve friends' photo from server: %1", job->errorText()));
    }
    else
    {
        // Create Item
        KABC::Addressee addressee = toPimAddressee(*user);
        addressee.setPhoto(KABC::Picture(job->photo()));
        Item newUser;
        newUser.setRemoteId(QString::number(user->uid()));
        newUser.setMimeType("text/directory");
        newUser.setPayload<KABC::Addressee>(addressee);
//        TimeStampAttribute * const timeStampAttribute = new TimeStampAttribute();
//        timeStampAttribute->setTimeStamp( user->updatedTime() );
//        newUser.addAttribute( timeStampAttribute );

        itemsRetrievedIncremental(Item::List() << newUser, Item::List());
        m_numPhotosFetched ++;

        if (m_numPhotosFetched != m_numFriends)
        {
            const int alreadyDownloadedFriends = m_numPhotosFetched;
            const float percentageDone = alreadyDownloadedFriends / (float)m_numFriends * 100.0f;
            emit percent(10 + percentageDone * 0.9f);
        }
        else
        {
            itemsRetrievalDone();
            finishFriendFetching();
        }
    }
}

void VkontakteResource::friendJobFinished(KJob *kjob)
{
    Q_ASSERT(!m_idle);
    Q_ASSERT(m_currentJobs.indexOf(kjob) != -1);
    Vkontakte::UserInfoFullJob * const job = dynamic_cast<Vkontakte::UserInfoFullJob*>(kjob);
    Q_ASSERT(job);
    Q_ASSERT(job->userInfo().size() == 1);
    m_currentJobs.removeAll(job);

    if (job->error())
    {
        abortWithError(i18n("Unable to get information about friend from server: %1", job->errorText()));
    }
    else
    {
        Item user = job->property("Item").value<Item>();
        user.setPayload<KABC::Addressee>(toPimAddressee(*job->userInfo().first()));
        // TODO: What about picture here?
        itemRetrieved(user);
        resetState();
    }
}
