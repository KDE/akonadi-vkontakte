#ifndef NEWSFEEDJOB_H
#define NEWSFEEDJOB_H

#include <libkvkontakte/vkontaktejobs.h>
#include <libkvkontakte/newsiteminfo.h>


class LIBKVKONTAKTE_EXPORT NewsfeedJob : public VkontakteJob
{
    Q_OBJECT
public:
    NewsfeedJob(const QString &accessToken);

    QList<NewsItemInfoPtr> newsInfo() const;
    // TODO: info about users and groups

protected:
    virtual void handleData(const QVariant &data);

    NewsItemInfoPtr handleSingleData(const QVariant &data);

private:
    QList<NewsItemInfoPtr> m_newsInfo;
};

#endif // NEWSFEEDJOB_H
