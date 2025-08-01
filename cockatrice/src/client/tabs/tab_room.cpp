#include "tab_room.h"

#include "../../client/game_logic/abstract_client.h"
#include "../../dialogs/dlg_settings.h"
#include "../../game/game_selector.h"
#include "../../main.h"
#include "../../server/chat_view/chat_view.h"
#include "../../server/pending_command.h"
#include "../../server/user/user_list_manager.h"
#include "../../server/user/user_list_widget.h"
#include "../../settings/cache_settings.h"
#include "get_pb_extension.h"
#include "pb/event_join_room.pb.h"
#include "pb/event_leave_room.pb.h"
#include "pb/event_list_games.pb.h"
#include "pb/event_remove_messages.pb.h"
#include "pb/event_room_say.pb.h"
#include "pb/room_commands.pb.h"
#include "pb/serverinfo_room.pb.h"
#include "tab_account.h"
#include "tab_supervisor.h"
#include "trice_limits.h"

#include <QApplication>
#include <QCompleter>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QSystemTrayIcon>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtCore/qdatetime.h>

TabRoom::TabRoom(TabSupervisor *_tabSupervisor,
                 AbstractClient *_client,
                 ServerInfo_User *_ownUser,
                 const ServerInfo_Room &info)
    : Tab(_tabSupervisor), client(_client), roomId(info.room_id()), roomName(QString::fromStdString(info.name())),
      ownUser(_ownUser), userListProxy(_tabSupervisor->getUserListManager())
{
    const int gameTypeListSize = info.gametype_list_size();
    for (int i = 0; i < gameTypeListSize; ++i)
        gameTypes.insert(info.gametype_list(i).game_type_id(),
                         QString::fromStdString(info.gametype_list(i).description()));

    QMap<int, GameTypeMap> tempMap;
    tempMap.insert(info.room_id(), gameTypes);
    gameSelector = new GameSelector(client, tabSupervisor, this, QMap<int, QString>(), tempMap, true, true);
    userList = new UserListWidget(tabSupervisor, client, UserListWidget::RoomList);
    connect(userList, SIGNAL(openMessageDialog(const QString &, bool)), this,
            SIGNAL(openMessageDialog(const QString &, bool)));

    chatView = new ChatView(tabSupervisor, nullptr, true, this);
    connect(chatView, &ChatView::showMentionPopup, this, &TabRoom::actShowMentionPopup);
    connect(chatView, &ChatView::messageClickedSignal, this, &TabRoom::focusTab);
    connect(chatView, &ChatView::openMessageDialog, this, &TabRoom::openMessageDialog);
    connect(chatView, &ChatView::showCardInfoPopup, this, &TabRoom::showCardInfoPopup);
    connect(chatView, &ChatView::deleteCardInfoPopup, this, &TabRoom::deleteCardInfoPopup);
    connect(chatView, &ChatView::addMentionTag, this, &TabRoom::addMentionTag);
    connect(&SettingsCache::instance(), &SettingsCache::chatMentionCompleterChanged, this,
            &TabRoom::actCompleterChanged);
    sayLabel = new QLabel;
    sayEdit = new LineEditCompleter;
    sayEdit->setMaxLength(MAX_TEXT_LENGTH);
    sayLabel->setBuddy(sayEdit);
    connect(sayEdit, &LineEditCompleter::returnPressed, this, &TabRoom::sendMessage);

    QMenu *chatSettingsMenu = new QMenu(this);

    aClearChat = chatSettingsMenu->addAction(QString());
    connect(aClearChat, &QAction::triggered, this, &TabRoom::actClearChat);

    chatSettingsMenu->addSeparator();

    aOpenChatSettings = chatSettingsMenu->addAction(QString());
    connect(aOpenChatSettings, &QAction::triggered, this, &TabRoom::actOpenChatSettings);

    QToolButton *chatSettingsButton = new QToolButton;
    chatSettingsButton->setIcon(QPixmap("theme:icons/settings"));
    chatSettingsButton->setMenu(chatSettingsMenu);
    chatSettingsButton->setPopupMode(QToolButton::InstantPopup);

    QHBoxLayout *sayHbox = new QHBoxLayout;
    sayHbox->addWidget(sayLabel);
    sayHbox->addWidget(sayEdit);
    sayHbox->addWidget(chatSettingsButton);

    QVBoxLayout *chatVbox = new QVBoxLayout;
    chatVbox->addWidget(chatView);
    chatVbox->addLayout(sayHbox);

    chatGroupBox = new QGroupBox;
    chatGroupBox->setLayout(chatVbox);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(gameSelector);
    splitter->addWidget(chatGroupBox);

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget(splitter, 3);
    hbox->addWidget(userList, 1);

    aLeaveRoom = new QAction(this);
    connect(aLeaveRoom, &QAction::triggered, this, &TabRoom::closeRequest);

    roomMenu = new QMenu(this);
    roomMenu->addAction(aLeaveRoom);
    addTabMenu(roomMenu);

    const int userListSize = info.user_list_size();
    for (int i = 0; i < userListSize; ++i) {
        userList->processUserInfo(info.user_list(i), true);
        autocompleteUserList.append("@" + QString::fromStdString(info.user_list(i).name()));
    }
    userList->sortItems();

    const int gameListSize = info.game_list_size();
    for (int i = 0; i < gameListSize; ++i)
        gameSelector->processGameInfo(info.game_list(i));

    completer = new QCompleter(autocompleteUserList, sayEdit);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setMaxVisibleItems(5);
    completer->setFilterMode(Qt::MatchStartsWith);

    sayEdit->setCompleter(completer);
    actCompleterChanged();
    connect(&SettingsCache::instance().shortcuts(), &ShortcutsSettings::shortCutChanged, this,
            &TabRoom::refreshShortcuts);
    refreshShortcuts();

    retranslateUi();

    QWidget *mainWidget = new QWidget(this);
    mainWidget->setLayout(hbox);
    setCentralWidget(mainWidget);
}

void TabRoom::retranslateUi()
{
    gameSelector->retranslateUi();
    chatView->retranslateUi();
    userList->retranslateUi();
    sayLabel->setText(tr("&Say:"));
    chatGroupBox->setTitle(tr("Chat"));
    roomMenu->setTitle(tr("&Room"));
    aLeaveRoom->setText(tr("&Leave room"));
    aClearChat->setText(tr("&Clear chat"));
    aOpenChatSettings->setText(tr("Chat Settings..."));
}

void TabRoom::focusTab()
{
    activateWindow();
    tabSupervisor->setCurrentIndex(tabSupervisor->indexOf(this));
    emit maximizeClient();
}

void TabRoom::actShowMentionPopup(const QString &sender)
{
    this->actShowPopup(sender + tr(" mentioned you."));
}

void TabRoom::actShowPopup(const QString &message)
{
    if (trayIcon && (tabSupervisor->currentIndex() != tabSupervisor->indexOf(this) ||
                     QApplication::activeWindow() == nullptr || QApplication::focusWidget() == nullptr)) {
        disconnect(trayIcon, &QSystemTrayIcon::messageClicked, nullptr, nullptr);
        trayIcon->showMessage(message, tr("Click to view"));
        connect(trayIcon, &QSystemTrayIcon::messageClicked, chatView, &ChatView::messageClickedSignal);
    }
}

void TabRoom::closeEvent(QCloseEvent *event)
{
    sendRoomCommand(prepareRoomCommand(Command_LeaveRoom()));
    emit roomClosing(this);
    event->accept();
}

void TabRoom::tabActivated()
{
    if (!sayEdit->hasFocus())
        sayEdit->setFocus();
}

QString TabRoom::sanitizeHtml(QString dirty) const
{
    return dirty.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;");
}

void TabRoom::sendMessage()
{
    if (sayEdit->text().isEmpty()) {
        return;
    } else if (completer->popup()->isVisible()) {
        completer->popup()->hide();
        return;
    } else {
        Command_RoomSay cmd;
        cmd.set_message(sayEdit->text().toStdString());

        PendingCommand *pend = prepareRoomCommand(cmd);
        connect(pend, &PendingCommand::finished, this, &TabRoom::sayFinished);
        sendRoomCommand(pend);
        sayEdit->clear();
    }
}

void TabRoom::sayFinished(const Response &response)
{
    if (response.response_code() == Response::RespChatFlood)
        chatView->appendMessage(tr("You are flooding the chat. Please wait a couple of seconds."));
}

void TabRoom::actClearChat()
{
    chatView->clearChat();
}

void TabRoom::actOpenChatSettings()
{
    DlgSettings settings(this);
    settings.setTab(4);
    settings.exec();
}

void TabRoom::actCompleterChanged()
{
    SettingsCache::instance().getChatMentionCompleter() ? completer->setCompletionRole(2)
                                                        : completer->setCompletionRole(1);
}

void TabRoom::processRoomEvent(const RoomEvent &event)
{
    switch (static_cast<RoomEvent::RoomEventType>(getPbExtension(event))) {
        case RoomEvent::LIST_GAMES:
            processListGamesEvent(event.GetExtension(Event_ListGames::ext));
            break;
        case RoomEvent::JOIN_ROOM:
            processJoinRoomEvent(event.GetExtension(Event_JoinRoom::ext));
            break;
        case RoomEvent::LEAVE_ROOM:
            processLeaveRoomEvent(event.GetExtension(Event_LeaveRoom::ext));
            break;
        case RoomEvent::ROOM_SAY:
            processRoomSayEvent(event.GetExtension(Event_RoomSay::ext));
            break;
        case RoomEvent::REMOVE_MESSAGES:
            processRemoveMessagesEvent(event.GetExtension(Event_RemoveMessages::ext));
            break;
        default:;
    }
}

void TabRoom::processListGamesEvent(const Event_ListGames &event)
{
    const int gameListSize = event.game_list_size();
    for (int i = 0; i < gameListSize; ++i)
        gameSelector->processGameInfo(event.game_list(i));
}

void TabRoom::processJoinRoomEvent(const Event_JoinRoom &event)
{
    userList->processUserInfo(event.user_info(), true);
    userList->sortItems();
    if (!autocompleteUserList.contains("@" + QString::fromStdString(event.user_info().name()))) {
        autocompleteUserList << "@" + QString::fromStdString(event.user_info().name());
        sayEdit->setCompletionList(autocompleteUserList);
    }
}

void TabRoom::processLeaveRoomEvent(const Event_LeaveRoom &event)
{
    userList->deleteUser(QString::fromStdString(event.name()));
    autocompleteUserList.removeOne("@" + QString::fromStdString(event.name()));
    sayEdit->setCompletionList(autocompleteUserList);
}

void TabRoom::processRoomSayEvent(const Event_RoomSay &event)
{
    QString senderName = QString::fromStdString(event.name());
    QString message = QString::fromStdString(event.message());

    if (userListProxy->isUserIgnored(senderName))
        return;

    UserListTWI *twi = userList->getUsers().value(senderName);
    ServerInfo_User userInfo = {};
    if (twi) {
        userInfo = twi->getUserInfo();
        if (SettingsCache::instance().getIgnoreUnregisteredUsers() &&
            !UserLevelFlags(userInfo.user_level()).testFlag(ServerInfo_User::IsRegistered))
            return;
    }

    if (event.message_type() == Event_RoomSay::ChatHistory && !SettingsCache::instance().getRoomHistory())
        return;

    if (event.message_type() == Event_RoomSay::ChatHistory)
        message =
            "[" +
            QString(QDateTime::fromMSecsSinceEpoch(event.time_of()).toLocalTime().toString("d MMM yyyy HH:mm:ss")) +
            "] " + message;

    chatView->appendMessage(message, event.message_type(), userInfo, true);
    emit userEvent(false);
}

void TabRoom::processRemoveMessagesEvent(const Event_RemoveMessages &event)
{
    QString userName = QString::fromStdString(event.name());
    int amount = event.amount();
    chatView->redactMessages(userName, amount);
}

void TabRoom::refreshShortcuts()
{
    aClearChat->setShortcuts(SettingsCache::instance().shortcuts().getShortcut("tab_room/aClearChat"));
}

void TabRoom::addMentionTag(QString mentionTag)
{
    sayEdit->insert(mentionTag + " ");
    sayEdit->setFocus();
}

PendingCommand *TabRoom::prepareRoomCommand(const ::google::protobuf::Message &cmd)
{
    return client->prepareRoomCommand(cmd, roomId);
}

void TabRoom::sendRoomCommand(PendingCommand *pend)
{
    client->sendCommand(pend);
}
