#include "stdafx.h"
#include "MiniChat.h"
#include <QMessageBox>
#include <sstream>
MiniChat::MiniChat(
    QString ip,
    QWidget* parent
)
    : QMainWindow(parent),
    serverIP(ip)
{
    ui.setupUi(this);

    ui.lblSessionRedDot->hide();
    ui.lblRequestRedDot->hide();
    client.OnRecentChatChanged =
        [this]()
        {
            QMetaObject::invokeMethod(
                this,
                [this]()
                {
                    LoadRecentChats();

                },
                Qt::QueuedConnection);
        };
    client.OnChatChanged =
        [this]()
        {
            QMetaObject::invokeMethod(
                this,
                [this]()
                {
                    if (!currentFriend.isEmpty())
                    {
                        LoadHistory(currentFriend);
                    }
                },
                Qt::QueuedConnection);
        };

    client.OnDisconnected =
        [this]()
        {
            QMetaObject::invokeMethod(
                this,
                [this]()
                {
                    QMessageBox::warning(
                        this,
                        "提示",
                        "服务器连接已断开！");

                    client.Disconnect();
                    ui.statusLabel->setText("网络异常，请重试！");
                    ResetUI();
                },
                Qt::QueuedConnection);
        };

    client.OnFriendRequestChanged =
        [this]()
        {
            QMetaObject::invokeMethod(
                this,
                [this]()
                {
                    UpdateFriendRequestNotify();
                    LoadFriendRequestList();
                },
                Qt::QueuedConnection);
        };
    client.OnAddFriendResult =
        [this](AddFriendResult reply)
        {
            QMetaObject::invokeMethod(
                this,
                [this, reply]()
                {
                    AddFriendAction(reply);
                },
         Qt::QueuedConnection);
   
        };

    client.OnLoginResult =
        [this](LoginResult reply)
        {
            QMetaObject::invokeMethod(
                this,
                [this, reply]()
                {
                    LoginAction(reply);
                },
                Qt::QueuedConnection);
        };

    client.OnRegisterResult =
        [this](RegisterResult reply)
        {
            QMetaObject::invokeMethod(
                this,
                [this, reply]()
                {
                    RegisterAction(reply);
                },
                Qt::QueuedConnection);
        };

    client.OnFriendListResult =
        [this](Message msg)
        {
            QMetaObject::invokeMethod(
                this,
                [this, msg]()
                {
                    FriendListAction(msg);
                },
                Qt::QueuedConnection);
        };

    client.OnFriendRequestListResult =
        [this](Message msg)
        {
            QMetaObject::invokeMethod(
                this,
                [this, msg]()
                {
                   FriendRequestListAction(msg);
                },
                Qt::QueuedConnection);
        };
    //右键菜单删除好友
    ui.FriendList->setContextMenuPolicy(
        Qt::CustomContextMenu);

    connect(ui.FriendList,
        &QListWidget::customContextMenuRequested,
        this,
        &MiniChat::OnFriendListMenu);
}

MiniChat::~MiniChat()
{
    client.Disconnect();
}

void MiniChat::on_loginButton_clicked()
{
    if (!EnsureConnected())
        return;

    QString user = ui.userEdit->text();

    QString pwd = ui.passwordEdit->text();

    client.Login(
            user.toStdString(),
            pwd.toStdString());
   
};
void MiniChat::on_registerButton_clicked()
{
    if (!EnsureConnected())
        return;

    QString user = ui.userEdit->text();

    QString pwd = ui.passwordEdit->text();

        client.Register(
            user.toStdString(),
            pwd.toStdString());
}
void MiniChat::on_btnSession_clicked()
{
    ui.contentWidget->setCurrentWidget(ui.pageSession);
   
    LoadRecentChats();

    CurrentChatClear();

}
void MiniChat::on_btnFriend_clicked()
{
    ui.contentWidget->setCurrentWidget(ui.pageFriend);

    LoadFriendList();

    CurrentChatClear();
}
void MiniChat::on_btnRequest_clicked()
{
    LoadFriendRequestList();

    client.ClearPendingNotify();

    UpdateFriendRequestNotify();

    ui.contentWidget->setCurrentWidget(ui.pageRequest);

    CurrentChatClear();
}

void MiniChat::on_btnLogout_clicked()
{
    QMessageBox msg(this);

    msg.setWindowTitle("退出登录");
    msg.setText("确定退出当前账号？");
    msg.setIcon(QMessageBox::Question);

    QPushButton* btnYes =
        msg.addButton("确定",
            QMessageBox::AcceptRole);

    QPushButton* btnNo =
        msg.addButton("取消",
            QMessageBox::RejectRole);

    msg.exec();

    if (msg.clickedButton() != btnYes)
    {
        return;
    }

    ui.statusLabel->setText(" ");

    client.Logout();

    client.Disconnect();

    ResetUI();
}

void MiniChat::LoadRecentChats()
{
    ui.sessionList->clear();

    auto chats = client.GetRecentChats();

    for (auto& chat : chats)
    {
        QString text;

        // 有未读消息
        if (client.HasNewMessage(chat.friendName))
        {
            text += QString::fromUtf8("🔴 ");
        }

        text += QString::fromStdString(chat.friendName);

        text += "\n";

        text += QString::fromStdString(chat.lastMessage);

        text += "\n";

        QString time =
            QString::fromStdString(chat.lastTime);

        QDateTime dt =
            QDateTime::fromString(
                time,
                "yyyy-MM-dd HH:mm:ss");

        time = dt.toString("MM-dd HH:mm");

        text += time;

        QListWidgetItem* item =
            new QListWidgetItem(text);
        // 保存真实好友名（用于点击进入聊天）
        item->setData(
            Qt::UserRole,
            QString::fromStdString(chat.friendName));
        ui.sessionList->addItem(item);
        

    }
    UpdateRecentChatNotify();
}
void MiniChat::UpdateRecentChatNotify()
{
    if (client.HasAnyNewMessage())
    {
        ui.lblSessionRedDot->show();
    }
    else
    {
        ui.lblSessionRedDot->hide();
    }
}
void MiniChat::on_sessionList_itemClicked(
    QListWidgetItem* item)
{
    QString friendName =
        item->data(Qt::UserRole).toString();

    currentFriend = friendName;

    ui.lblFriendName->setText(friendName);

    client.OpenChat(friendName.toStdString());

    LoadHistory(friendName);

    ui.contentWidget->setCurrentWidget(ui.pageChat);

    // 更新消息提醒
    UpdateRecentChatNotify();
}
void MiniChat::on_btnBack_clicked()
{
    ui.contentWidget->setCurrentWidget(ui.pageSession);

    LoadRecentChats();

    CurrentChatClear();
}

void MiniChat::LoadHistory(
    const QString& friendName)
{
    ui.listChat->clear();

    auto history =
        client.GetHistory(
            friendName.toStdString());

    QDateTime lastShowTime;
    bool firstMessage = true;

    for (auto& msg : history)
    {
        // 当前消息时间
        QDateTime currentTime =
            QDateTime::fromString(
                QString::fromStdString(msg.time),
                "yyyy-MM-dd HH:mm:ss");

        // ===== 时间单独作为一个Item =====
        if (firstMessage ||
            lastShowTime.secsTo(currentTime) >= 600)
        {
            QListWidgetItem* timeItem =
                new QListWidgetItem(
                    currentTime.toString("MM-dd HH:mm"));

            timeItem->setForeground(Qt::gray);

            timeItem->setTextAlignment(Qt::AlignCenter);

            ui.listChat->addItem(timeItem);

            lastShowTime = currentTime;
            firstMessage = false;
        }

        // ===== 消息Item =====
        QString line;

        if (msg.sender == client.GetUsername())
        {
            line =
                "我："
                + QString::fromStdString(msg.content);
        }
        else
        {
            line =
                QString::fromStdString(msg.sender)
                + "："
                + QString::fromStdString(msg.content);
        }

        ui.listChat->addItem(line);
    }

    ui.listChat->scrollToBottom();
}
void MiniChat::on_btnSend_clicked()
{
    QString text =
        ui.editSend->toPlainText();

    if (text.isEmpty())
        return;

    client.SendChatMessage(
        text.toStdString());

    ui.editSend->clear();

    LoadHistory(currentFriend);
}
void MiniChat::on_deleteChat_clicked()
{
    if (currentFriend.isEmpty())
        return;

    QMessageBox msg(this);

    msg.setWindowTitle("删除聊天");

    msg.setText(
        "确定删除与 "
        + currentFriend
        + " 的全部聊天记录？");

    msg.setIcon(QMessageBox::Question);

    QPushButton* btnYes =
        msg.addButton(
            "确定",
            QMessageBox::AcceptRole);

    QPushButton* btnNo =
        msg.addButton(
            "取消",
            QMessageBox::RejectRole);

    msg.exec();


    if (msg.clickedButton() != btnYes)
    {
        return;
    }


    client.DeleteChatHistory(
        currentFriend.toStdString());

        ui.listChat->clear();

        LoadRecentChats();

}
void MiniChat::LoadFriendList()
{
    ui.FriendList->clear();

    // 获取好友列表
   client.GetFriendList();

}
void MiniChat::on_FriendList_itemClicked(
    QListWidgetItem* item)
{
    QString friendName =
        item->data(Qt::UserRole).toString();

    currentFriend = friendName;

    ui.lblFriendName->setText(friendName);

    client.OpenChat(friendName.toStdString());

    LoadHistory(friendName);

    ui.contentWidget->setCurrentWidget(ui.pageChat);

    // 更新消息提醒
    UpdateRecentChatNotify();
}

void MiniChat::ResetUI()
{
    currentFriend.clear();

    ui.userEdit->clear();
    ui.passwordEdit->clear();

    ui.sessionList->clear();
    ui.FriendList->clear();
    ui.listChat->clear();

    ui.lblFriendName->clear();
    ui.labelUser->clear();

    ui.lblSessionRedDot->hide();
    ui.lblRequestRedDot->hide();

    ui.contentWidget->setCurrentWidget(ui.pageSession);

    ui.stackedWidget->setCurrentIndex(0);
}

bool MiniChat::EnsureConnected()
{
    if (client.IsConnected())
        return true;

    if (!client.Connect(
        serverIP.toStdString(),
        8888))
    {
        client.Disconnect();
        ui.statusLabel->setText("网络异常，请重试！");
        QMessageBox::critical(
            this,
            "连接服务器",
            "无法连接服务器！");
        return false;
    }
    ui.statusLabel->setText("已连接到服务器！");
    return true;
}

void MiniChat::on_QuestSend_clicked()
{
    QString friendName =
        ui.FriendEdit->text().trimmed();

    ui.FriendEdit->clear();

    if (friendName.isEmpty())
    {
        QMessageBox::warning(
            this,
            "提示",
            "请输入好友账号！");
        return;
    }

    AddFriendResult result =
        client.AddFriend(
            friendName.toStdString());

};

void MiniChat::LoadFriendRequestList()
{
    ui.FriendQuestList->clear();

   client.GetFriendRequests();

}

void MiniChat::on_FriendQuestList_itemClicked(
    QListWidgetItem* item)
{

    QString sender =
        item->data(Qt::UserRole).toString();

    QString receiver =
        item->data(Qt::UserRole + 1).toString();

    int status =
        item->data(Qt::UserRole + 2).toInt();

   
    // 只有别人申请我，且待处理
    if (receiver != client.GetUsername())
    {
        
        return;
    }

    if (status != 0)
    {
       
        return;
    }

    QMessageBox msg(this);

    msg.setWindowTitle("好友申请");

    msg.setText(sender + " 请求添加你为好友");

    QPushButton* btnAccept =
        msg.addButton("同意",
            QMessageBox::AcceptRole);

    QPushButton* btnReject =
        msg.addButton("拒绝",
            QMessageBox::RejectRole);

    msg.addButton("取消",
        QMessageBox::RejectRole);

    msg.exec();

    if (msg.clickedButton() == btnAccept)
    {
        client.AcceptFriend(
            sender.toStdString());
       

            LoadFriendRequestList();
    
    }
    else if (msg.clickedButton() == btnReject)
    {
        client.RejectFriend(
            sender.toStdString());
      

            LoadFriendRequestList();

    }

     client.ClearPendingNotify();

     UpdateFriendRequestNotify();
    return;
}

void MiniChat::UpdateFriendRequestNotify()
{
    if (client.HasPendingNotify())
    {
        ui.lblRequestRedDot->show();
    }
    else
    {
        ui.lblRequestRedDot->hide();
    }
}
void MiniChat::OnFriendListMenu(
    const QPoint& pos)
{
    // 获取鼠标右键所在的好友
    QListWidgetItem* item =
        ui.FriendList->itemAt(pos);

    if (item == nullptr)
    {
        return;
    }

    QString friendName =
        item->data(Qt::UserRole).toString();

    QMenu menu(this);

    QFont font;
    font.setPointSize(16);

    menu.setFont(font);
    QAction* deleteAction =
        menu.addAction("删除好友");

    QAction* action =
        menu.exec(
            ui.FriendList->viewport()->mapToGlobal(pos));

    if (action != deleteAction)
    {
        return;
    }

    // 弹确认框
    QMessageBox msg(this);

    msg.setWindowTitle("删除好友");

    msg.setText(
        "确定删除好友 "
        + friendName
        + " 吗？");

    QPushButton* btnDelete =
        msg.addButton(
            "确定",
            QMessageBox::AcceptRole);

    msg.addButton(
        "取消",
        QMessageBox::RejectRole);

    msg.exec();

    if (msg.clickedButton() != btnDelete)
    {
        return;
    }

    // 删除
    client.DeleteFriend(
        friendName.toStdString());
   

        LoadFriendList();

        client.ClearNewMessage(friendName.toStdString());

        UpdateRecentChatNotify();

}
void MiniChat::CurrentChatClear()
{

    client.CloseChat();

    currentFriend.clear();

    ui.listChat->clear();

    ui.lblFriendName->clear();
}
void MiniChat::AddFriendAction(AddFriendResult reply)
{
    switch (reply)
    {
        case AddFriendResult::Success:
        {
            QMessageBox::information(
                this,
                "提示",
                "好友申请已发送！");

            ui.FriendQuestList->clear();

            client.GetFriendRequests();

            break;
        }
        case AddFriendResult::UserNotExist:
        {
            QMessageBox::warning(
                this,
                "提示",
                "用户不存在！");
            break;
        }
        case AddFriendResult::AlreadyFriend:
        {
            QMessageBox::warning(
                this,
                "提示",
                "你们已经是好友！");
            break;
        }
        case AddFriendResult::AlreadySent:
        {
            QMessageBox::warning(
                this,
                "提示",
                "已有好友申请！");
            break;
        }
        case AddFriendResult::Self:
        {
            QMessageBox::warning(
                this,
                "提示",
                "不能添加自己为好友！");
            break;
        }
        case AddFriendResult::DatabaseError:
        {
            QMessageBox::warning(
                this,
                "提示",
                "服务器错误，请重试！");
            break;
        }
    }
}
void MiniChat::LoginAction(LoginResult reply)
{
    switch (reply)
    {
        case LoginResult::Success:
        {
            ui.stackedWidget->setCurrentIndex(1);

            ui.labelUser->setText(QString::fromStdString(client.username));

            LoadRecentChats();

            UpdateFriendRequestNotify();

            break;
        }

        case LoginResult::UserNotExist:
        {
            QMessageBox::warning(
                this,
                "提示",
                "用户不存在");

            break;
        }

        case LoginResult::PasswordError:
        {
            QMessageBox::warning(
                this,
                "提示",
                "密码错误");

            break;
        }

        case LoginResult::NetworkError:
        {
            QMessageBox::warning(
                this,
                "提示",
                "网络异常");

            break;
        }
        case LoginResult::AlreadyOnline:
        {
            QMessageBox::warning(
                this,
                "提示",
                "该账号已登录");

            break;
        }
    }
}
void MiniChat::RegisterAction(RegisterResult reply)
{
    switch (reply)
    {
    case RegisterResult::Success:
        QMessageBox::information(
            this,
            "提示",
            "注册成功");
        break;

    case RegisterResult::UserAlreadyExist:
        QMessageBox::warning(
            this,
            "提示",
            "用户名已存在");
        break;

    case RegisterResult::DatabaseError:
        QMessageBox::warning(
            this,
            "提示",
            "服务器错误，请重试");
        break;

    case RegisterResult::NetworkError:
        QMessageBox::warning(
            this,
            "提示",
            "网络异常，请重试");
        break;
    }
}
void MiniChat::FriendListAction(Message msg)
{
    std::vector<std::string> friends;

    std::stringstream ss(msg.text);

    std::string name;

    while (getline(ss, name, '|'))
    {
        if (!name.empty())
        {
            friends.push_back(name);
        }
    }

    for (const auto& name : friends)
    {
        QListWidgetItem* item =
            new QListWidgetItem(
                QString::fromStdString(name));

        // 保存真实好友名
        item->setData(
            Qt::UserRole,
            QString::fromStdString(name));

        ui.FriendList->addItem(item);
    }
}
void MiniChat::FriendRequestListAction(Message msg)
{
    std::vector<FriendRequest> requests;

    std::stringstream ss(msg.text);

    std::string item;

    while (getline(ss, item, '|'))
    {
        if (item.empty())
            continue;

        FriendRequest req;

        std::stringstream line(item);

        std::string status;

        getline(line, req.sender, ',');

        getline(line, req.receiver, ',');

        getline(line, status, ',');

        req.status = atoi(status.c_str());

        requests.push_back(req);
    }
    for (auto& req : requests)
    {
        QString text;
        // 只有别人申请我且未处理，才显示红点
        if (req.receiver == client.GetUsername() &&
            req.status == 0)
        {
            text += QString::fromUtf8("🔴 ");
        }

        // 显示名字
        if (req.receiver == client.GetUsername())
        {
            text += QString::fromStdString(req.sender);
        }
        else
        {
            text += QString::fromStdString(req.receiver);
        }

        text += "\n";

        // 状态
        if (req.receiver == client.GetUsername())
        {
            // 别人申请我

            if (req.status == 0)
            {
                text += "申请添加好友";
            }
            else if (req.status == 1)
            {
                text += "我已同意";
            }
            else
            {
                text += "我已拒绝";
            }
        }
        else
        {
            // 我申请别人

            if (req.status == 0)
            {
                text += "等待对方处理";
            }
            else if (req.status == 1)
            {
                text += "对方已同意";
            }
            else
            {
                text += "对方已拒绝";
            }
        }

        QListWidgetItem* item =
            new QListWidgetItem(text);

        // 保存真实数据（后面点击要用）
        item->setData(
            Qt::UserRole,
            QString::fromStdString(req.sender));

        item->setData(
            Qt::UserRole + 1,
            QString::fromStdString(req.receiver));

        item->setData(
            Qt::UserRole + 2,
            req.status);

        ui.FriendQuestList->addItem(item);
    }
}