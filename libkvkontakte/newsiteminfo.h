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
#ifndef NEWSITEMINFO_H
#define NEWSITEMINFO_H

#include "libkvkontakte_export.h"

#include <QSharedPointer>

#include <krss/item.h>
#include "qintlist.h"

/**
 * For items of array in "items" field in the response to "newsfeed.get" method.
 * 
 * http://vkontakte.ru/developers.php?o=-1&p=newsfeed.get
 */
class LIBKVKONTAKTE_EXPORT NewsItemInfo : public QObject
{
    Q_OBJECT

    /**
     * Type of news item, one of the following: (the same as in "filters")
     *  - post (new wall entries)
     *  - photo (new photos)
     *  - photo_tag (new people tagged in photos)
     *  - friend (new friends)
     *  - note (new notes)
     */
    Q_PROPERTY(QString type WRITE setType READ type)

    /**
     * Identifier of news source.
     * Positive if it is a user ID.
     * Negative if it is a group ID.
     */
    Q_PROPERTY(int source_id WRITE setSourceId READ sourceId)

//     date - время публикации новости в формате unixtime
// 
//     post_id - находится в записях со стен и содержит идентификатор записи на стене владельца
// 
//     copy_owner_id - находится в записях со стен, если сообщение является копией сообщения с чужой стены, и содержит идентификатор владельца стены, у которого было скопировано сообщение
// 
//     copy_post_id - находится в записях со стен, если сообщение является копией сообщения с чужой стены, и содержит идентификатор скопированного сообщения на стене его владельца
// 
//     text - находится в записях со стен и содержит текст записи
// 
//     likes - находится в записях со стен и содержит информацию о числе людей, которым понравилась данная запись, и понравился ли он текущему пользователю
// 
//     comments - находится в записях со стен и содержит информацию о количестве комментариев к записи и возможности комментирования записи текущим пользователем
// 
//     attachment - находится в записях со стен и содержит объект, который присоединен к текущей новости (фотография, ссылка и т.п.). Более подробная информация представлена на странице Описание поля attachment.
// 
//     geo - находится в записях со стен, в которых имеется информация о местоположении. Более подробная информация представлена на странице Описание поля geo
// 
//     photos, photo_tags, notes, friends - находятся в объектах соответствующих типов (кроме записей со стен) и содержат информацию о количестве объектов и до 5 последних объектов, связанных с данной новостью.
// 
//     // Каждый из элементов массива в полях photos и photo_tags содержит поля:
//         pid - идентификатор фотографии    
//         owner_id - идентификатор владельца фотографии
//         aid - идентификатор альбома
//         src - адрес изображения для предпросмотра
//         src_big - адрес полноразмерного изображения
//     // Каждый из элементов массива в поле notes содержит поля:
//         nid - идентификатор заметки
//         owner_id - идентификатор владельца заметки
//         title - заголовок заметки
//         ncom - количество комментариев к заметке

    /**
     * Every item of the array in field "friends" contains
     * a field "uid" (identifier of the friend).
     */
    Q_PROPERTY(QIntList friends WRITE setFriends READ friends)

public:
    NewsItemInfo();

    void setType(const QString &type);
    QString type() const;

    void setSourceId(int sourceId);
    int sourceId() const;

    void setTotalFriends(int totalFriends);
    void addFriendUid(int uid);
    void setFriends(const QIntList &friends);
    QIntList friends() const;

    QString remoteId() const;

    // TODO: KRss::Item::Ptr
    KRss::Item::Ptr asItem() const;

private:
    QString m_type;
    int m_sourceId;

    int m_totalFriends;
    QIntList m_friends;
};

typedef QSharedPointer<NewsItemInfo> NewsItemInfoPtr;

#endif // NEWSITEMINFO_H
