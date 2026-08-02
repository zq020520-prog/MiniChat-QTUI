#include <QtWidgets>
#include "ChatClient.h"
#include "Message.h"
#include <sstream>
#include <iostream>
#include <ws2tcpip.h>
#include "MiniChat.h"
#pragma comment(lib,"ws2_32.lib")

using namespace std;

ChatClient::ChatClient()
{
    sock = INVALID_SOCKET;

    running = false;
}

ChatClient::~ChatClient()
{
    Disconnect();
}

bool ChatClient::Connect(const std::string& ip,
    unsigned short port)
{
    WSADATA wsa;
    manualDisconnect = false;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {

        return false;
    }

    sock = socket(AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP);

    if (sock == INVALID_SOCKET)
    {

        return false;
    }

    sockaddr_in addr{};

    addr.sin_family = AF_INET;

    addr.sin_port = htons(port);

    InetPtonA(AF_INET,
        ip.c_str(),
        &addr.sin_addr);

    if (connect(sock,
        (sockaddr*)&addr,
        sizeof(addr))
        == SOCKET_ERROR)
    {

        closesocket(sock);

        return false;
    }

    running = true;

    recvThread = std::thread(
        &ChatClient::ReceiveLoop,
        this);

    return true;
}



void ChatClient::ReceiveLoop()
{
    while (running)
    {
        Message msg{};

        int len = recv(sock,
            (char*)&msg,
            sizeof(msg),
            0);

        if (len <= 0)
        {
            running = false;

            // 只有不是主动断开才通知UI
            if (!manualDisconnect && OnDisconnected)
            {

                OnDisconnected();
            }

            break;
        }

        switch (msg.type)
        {
            // 聊天消息

        case MessageType::CHAT:

        {
            chatDB.InsertMessage(
                msg.sender,
                username,
                msg.text);

            if (currentChatFriend != msg.sender)
            {

                chatDB.SetNewMessage(msg.sender);
            }

            if (OnRecentChatChanged)
            {
                OnRecentChatChanged();
            }
            if (OnChatChanged)
            {
                OnChatChanged();
            }


            break;
        }
        case MessageType::CHAT_RESULT:
        {
            if (!msg.result)
            {
                chatDB.InsertMessage(
                    username,
                    currentChatFriend,
                    "[发送失败] 你不是他的好友");
                if (OnRecentChatChanged)
                {
                    OnRecentChatChanged();
                }
                if (OnChatChanged)
                {
                    OnChatChanged();
                }
            }

            break;
        }
        case MessageType::PENDING_NOTIFY:
        {

            chatDB.SetPendingNotify();
            if (OnFriendRequestChanged)
            {
                OnFriendRequestChanged();
            }
            break;
        }
        case MessageType::FRIEND_LIST:
        {
            if (OnFriendListResult)
            {
                OnFriendListResult(msg);
            }
           
            break;
        }
        case MessageType::FRIEND_REQUEST_LIST:
        {
            if (OnFriendRequestListResult)
            {
                OnFriendRequestListResult(msg);
            }
           
            break;
        }
        case MessageType::ADD_FRIEND_RESULT:
        {

            AddFriendResult  reply = (AddFriendResult)msg.result;

            if (OnAddFriendResult)
            {
                OnAddFriendResult(
                    static_cast<AddFriendResult>(reply));
            }
            break;
        }
        case MessageType::LOGIN_RESULT:
        {

            if (msg.result == (int)LoginResult::Success)
            {
                username = msg.receiver;

                std::string dbName =
                    username + "_chat.db";

                if (chatDB.Open(dbName))
                    chatDB.CreateTable();
           
                Message ready{};

                ready.type = MessageType::READY;

                strcpy_s(ready.sender,username.c_str());

                send(sock,(char*)&ready,sizeof(ready), 0);
            }

            LoginResult reply=(LoginResult)msg.result;

            if (OnLoginResult)
            {
                OnLoginResult(
                    (LoginResult)reply);
            }
            break;
        }
        case MessageType::REGISTER_RESULT:
        {
           
            RegisterResult reply=(RegisterResult)msg.result;

            if (OnRegisterResult)
            {
                OnRegisterResult(
                    (RegisterResult)reply);
            }
            
            break;
        }

        //=========================
        // 其它消息（登录、好友等）
        //=========================
        default:
       
            break;

        }
    }
}

LoginResult ChatClient::Login(
    const std::string& user,
    const std::string& password)
{
    Message msg;

    msg.type = MessageType::LOGIN;

    strcpy_s(msg.sender, user.c_str());


    strcpy_s(msg.password, password.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);

    return (LoginResult)msg.result;
}
RegisterResult ChatClient::Register(
    const std::string& user,
    const std::string& password)
{
    Message msg{};

    msg.type = MessageType::REGISTER;

    strcpy_s(msg.sender, user.c_str());

    strcpy_s(msg.password, password.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);

    return (RegisterResult)msg.result;
}

std::vector<RecentChat> ChatClient::GetRecentChats()
{
    return chatDB.GetRecentChats(username);
}

void ChatClient::Disconnect()
{
    manualDisconnect = true;

    running = false;

    if (sock != INVALID_SOCKET)
    {
        shutdown(sock, SD_BOTH);   // 先唤醒recv（推荐）
        closesocket(sock);

        sock = INVALID_SOCKET;
    }

    if (recvThread.joinable())
    {
        recvThread.join();
    }

    WSACleanup();
}

AddFriendResult ChatClient::AddFriend(
    const std::string& friendName)
{
    Message msg{};

    msg.type = MessageType::ADD_FRIEND;

    strcpy_s(msg.sender,
        username.c_str());

    strcpy_s(msg.receiver,
        friendName.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);
  
    return (AddFriendResult)msg.result;
}


std::vector<std::string> ChatClient::GetFriendList()
{
    std::vector<std::string> friends;

    Message msg{};

    msg.type = MessageType::FRIEND_LIST;

    strcpy_s(msg.sender,
        username.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);

    return friends;
}

void ChatClient::Logout()
{
    Message msg{};

    msg.type = MessageType::LOGOUT;

    strcpy_s(msg.sender,
        username.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);

    chatting = false;

    currentChatFriend.clear();

    username.clear();

    chatDB.Close();
}


std::vector<FriendRequest> ChatClient::GetFriendRequests()
{
    std::vector<FriendRequest> requests;

    Message msg{};

    msg.type = MessageType::FRIEND_REQUEST_LIST;

    strcpy_s(msg.sender, username.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);

    return requests;
}
bool ChatClient::AcceptFriend(const std::string& sender)
{
    Message msg{};

    msg.type = MessageType::ACCEPT_FRIEND;

    // sender 是好友申请发起人，例如 Alice
    strcpy_s(msg.sender, sender.c_str());

    // receiver 是当前登录用户，例如 Bob
    strcpy_s(msg.receiver, username.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);
  
        // 自动发送第一条消息
        Message chat{};

        chat.type = MessageType::CHAT;

        strcpy_s(chat.sender, username.c_str());      // 当前用户(Bob)

        strcpy_s(chat.receiver, sender.c_str());      // Alice

        strcpy_s(chat.text, "我同意了你的好友申请，可以开始聊天了！");

        send(sock,
            (char*)&chat,
            sizeof(chat),
            0);

        // 保存到自己的聊天记录
        chatDB.InsertMessage(
            username,
            sender,
            "我同意了你的好友申请，可以开始聊天了！");

        return true;

}

bool ChatClient::RejectFriend(const std::string& sender)
{
    Message msg{};

    msg.type = MessageType::REJECT_FRIEND;

    // sender：申请发起人
    strcpy_s(msg.sender, sender.c_str());

    // receiver：当前登录用户
    strcpy_s(msg.receiver, username.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);

    return true;
  
}


void ChatClient::SendChatMessage(const std::string& text)
{
    Message msg{};

    msg.type = MessageType::CHAT;

    strcpy_s(msg.sender,
        username.c_str());

    strcpy_s(msg.receiver,
        currentChatFriend.c_str());

    strcpy_s(msg.text,
        text.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);

    chatDB.InsertMessage(
        username,
        currentChatFriend,
        text);
}


bool ChatClient::DeleteFriend(
    const std::string& friendName)
{
    Message msg{};

    msg.type =
        MessageType::DELETE_FRIEND;

    strcpy_s(
        msg.sender,
        username.c_str());

    strcpy_s(
        msg.receiver,
        friendName.c_str());

    send(sock,
        (char*)&msg,
        sizeof(msg),
        0);
 
    // 删除本地聊天记录
    chatDB.DeleteHistory(
        username,
        friendName);

    return true;
}
bool ChatClient::HasAnyNewMessage()
{
    return chatDB.HasAnyNewMessage();
}
bool ChatClient::HasNewMessage(
    const std::string& friendName)
{
    return chatDB.HasNewMessage( friendName);
}
void ChatClient::OpenChat(
    const std::string& friendName)
{
    std::lock_guard<std::mutex> lock(chatMutex);

    currentChatFriend = friendName;

    chatDB.ClearNewMessage(friendName);
}
std::vector<ChatRecord> ChatClient::GetHistory(
    const std::string& friendName)
{
    return chatDB.GetHistory(
        username,
        friendName);
}
std::string ChatClient::GetUsername() const
{
    return username;
}
void ChatClient::CloseChat()
{
    std::lock_guard<std::mutex> lock(chatMutex);

    currentChatFriend.clear();
}
bool ChatClient::DeleteChatHistory(
    const std::string& friendName)
{
    return chatDB.DeleteHistory(
        username,
        friendName);
}
bool ChatClient::IsConnected() const
{
    return sock != INVALID_SOCKET;
}
bool ChatClient::HasPendingNotify()
{
    return chatDB.HasPendingNotify();
}
void ChatClient::ClearPendingNotify()
{
    chatDB.ClearPendingNotify();
}
bool ChatClient::ClearNewMessage(
    const std::string& friendName)
{
    return chatDB.ClearNewMessage(friendName);
}