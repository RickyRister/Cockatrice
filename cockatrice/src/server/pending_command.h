#ifndef PENDING_COMMAND_H
#define PENDING_COMMAND_H

#include "pb/commands.pb.h"
#include "pb/response.pb.h"

#include <QVariant>

class PendingCommand : public QObject
{
    Q_OBJECT
signals:
    void finished(const Response &response, const CommandContainer &commandContainer, const QVariant &extraData);

private:
    CommandContainer commandContainer;
    QVariant extraData;
    int ticks;

public:
    explicit PendingCommand(const CommandContainer &_commandContainer, QVariant _extraData = QVariant());
    CommandContainer &getCommandContainer();
    void setExtraData(const QVariant &_extraData);
    QVariant getExtraData() const;
    void processResponse(const Response &response);
    int tick();
};

#endif
