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
#include "newsiteminfo.h"

NewsItemInfo::NewsItemInfo()
    : m_sourceId(0)
{
}

QString NewsItemInfo::type() const
{
    return m_type;
}

void NewsItemInfo::setType(const QString &type)
{
    m_type = type;
}

int NewsItemInfo::sourceId() const
{
    return m_sourceId;
}

void NewsItemInfo::setSourceId(int sourceId)
{
    m_sourceId = sourceId;
}

void NewsItemInfo::setTotalFriends(int totalFriends)
{
    m_totalFriends = totalFriends;
}

void NewsItemInfo::addFriendUid(int uid)
{
    m_friends.append(uid);
}

void NewsItemInfo::setFriends(const QIntList& friends)
{
    m_friends = friends;
}

QIntList NewsItemInfo::friends() const
{
    return m_friends;
}

QString NewsItemInfo::remoteId() const
{
    // FIXME: constuct remote ID from a mix of "source_id" and "date"
    return QString("feed%1").arg(rand());
}

KRss::Item::Ptr NewsItemInfo::asItem() const
{
    KRss::Item *item = new KRss::Item();

    if (type() == "friend")
    {
        item->setContent(QString("%1 has added %2 (%3 people total) to friends.").arg(sourceId()).arg(m_friends.join()).arg(m_totalFriends));
    }
    else
    {
        item->setContent("I have no idea how to handle this type of news.");
    }

    return KRss::Item::Ptr(item);
}
