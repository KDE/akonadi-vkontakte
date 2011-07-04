#include "newsfeedjob.h"

#include <qjson/qobjecthelper.h>

NewsfeedJob::NewsfeedJob(const QString &accessToken)
    : VkontakteJob("newsfeed.get", accessToken)
{
    // not setting "source_ids", because we want all news
    // not setting "fields", because we want all news
    
    // not setting "start_time", it will be set to 1 day ago
    // not setting "end_time", it will be set to current time
    addQueryItem("count", "100"); // 100 is maximum allowed number of news
}

NewsItemInfoPtr NewsfeedJob::handleSingleData(const QVariant &data)
{
    NewsItemInfoPtr newsInfo = NewsItemInfoPtr(new NewsItemInfo());
    QMap<QString, QVariant> map = data.toMap();

    QVariantList friends = map["friends"].toList();
    if (map.contains("friends") && friends.size() >= 1)
    {
        newsInfo->setTotalFriends(friends.first().toInt());
        friends.pop_front();
        foreach (const QVariant &item, friends) {
            newsInfo->addFriendUid(item.toMap()["uid"].toInt());
        }
        map.remove("friends"); // already handled
    }

    QJson::QObjectHelper::qvariant2qobject(map, newsInfo.data());
    return newsInfo;
}

void NewsfeedJob::handleData(const QVariant &data)
{
    // TODO: handle other info
    QVariantList items = data.toMap()["items"].toList();
    foreach (const QVariant &item, items) {
        m_newsInfo.append(handleSingleData(item));
    }
}

QList<NewsItemInfoPtr> NewsfeedJob::newsInfo() const
{
    return m_newsInfo;
}
