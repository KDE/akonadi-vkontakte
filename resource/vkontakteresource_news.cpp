/* Copyright 2011 Alexander Potashev <aspotashev@gmail.com>

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
#include <libkvkontakte/vkontaktejobs.h>
#include <libkvkontakte/newsfeedjob.h>

using namespace Akonadi;

void VkontakteResource::newsListFetched(KJob *job)
{
    Q_ASSERT( !m_idle );
    NewsfeedJob * const listJob = dynamic_cast<NewsfeedJob*>(job);
    Q_ASSERT( listJob );
    m_currentJobs.removeAll(job);

    if (listJob->error()) {
        abortWithError( i18n( "Unable to get news from server: %1", listJob->errorString() ),
                        listJob->error() == VkontakteJob::AuthenticationProblem );
        return;
    }


    setItemStreamingEnabled( true ); // how does this work???

    QList<NewsItemInfoPtr> allNews = listJob->newsInfo();

    Item::List items;
    foreach (const NewsItemInfoPtr &newsInfo, allNews)
    {
        KRss::Item::Ptr feedItem = newsInfo->asItem();

        Item item;
//        item.setRemoteId( newsInfo->remoteId() );
        item.setPayload<KRss::Item::Ptr>( feedItem );
        //item.setSize( /*feedItem->size()*/ 123 );
        item.setMimeType( KRss::Item::mimeType() );
        items.append(item);
    }

    itemsRetrieved(items);
//    itemsRetrieved(Item::List());
    itemsRetrievalDone();
    finishNewsFetching();
}

void VkontakteResource::finishNewsFetching()
{
    emit percent(100);
    emit status( Idle, i18n( "All news fetched from server." ) );
    resetState();
}
