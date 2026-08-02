#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MiniChat.h"
#include "ChatClient.h"

class MiniChat : public QMainWindow
{
    Q_OBJECT

public:
    MiniChat(
        QString ip,
        QWidget* parent = nullptr);

    ~MiniChat();
private slots:
    void on_loginButton_clicked();
    void on_registerButton_clicked();
    void on_btnSession_clicked();
    void on_btnFriend_clicked();
    void on_btnRequest_clicked();
    void on_btnLogout_clicked();
    void on_sessionList_itemClicked(
        QListWidgetItem* item);
    void on_btnBack_clicked();
    void on_btnSend_clicked();
    void on_FriendList_itemClicked(
        QListWidgetItem* item);
    void on_deleteChat_clicked();
    void on_QuestSend_clicked();

    void on_FriendQuestList_itemClicked(
        QListWidgetItem* item);

    void OnFriendListMenu(const QPoint& pos);


private:
    Ui::MiniChatClass ui;
    ChatClient client;

    QString serverIP;   // 保存服务器IP

    void ResetUI();
    void UpdateFriendRequestNotify();
    bool EnsureConnected();
    void LoadRecentChats();
    void LoadFriendList();
    void LoadFriendRequestList();
    void UpdateRecentChatNotify();

    void AddFriendAction(AddFriendResult reply);

    void LoginAction(LoginResult reply);

    void RegisterAction(RegisterResult reply);

    void FriendListAction(Message msg);

    void FriendRequestListAction(Message msg);

    void CurrentChatClear();
private:

    QString currentFriend;

    void LoadHistory(const QString& friendName);

    void OpenChat(const QString& friendName);
};

