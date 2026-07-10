#pragma once
#include <QtWidgets>
#include <winsock2.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include "Message.h"
#include <queue>
#include "ChatDatabase.h"
#include <functional>

struct FriendRequest
{
    std::string sender;      // 申请人
    std::string receiver;    // 接收人
    int status;              // 0待处理 1同意 2拒绝
};


class ChatClient
{
public:

   
    ChatClient();

    ~ChatClient();
    std::function<void()> OnChatChanged;

    bool Connect(const std::string& ip,
        unsigned short port);

    std::vector<RecentChat> GetRecentChats();
    bool DeleteChatHistory(
        const std::string& friendName);
    bool DeleteFriend(
        const std::string& friendName);

    bool AddFriend(
        const std::string& friendName);


    void CloseChat();
    bool Login(
        const std::string& user,
        const std::string& password);

    bool Register(
        const std::string& user,
        const std::string& password);

   
    bool HasPendingNotify();

    void Disconnect();

    // 同意好友申请
    bool AcceptFriend(const std::string& sender);
    void OpenChat(const std::string& friendName);
    // 拒绝好友申请
    bool RejectFriend(const std::string& sender);

    bool HasAnyNewMessage();
    bool HasNewMessage(const std::string& friendName);

    std::function<void()> OnRecentChatChanged;
    std::function<void()> OnFriendRequestChanged;

    std::vector<std::string> GetFriendList();
    // 获取好友申请
    std::vector<FriendRequest> GetFriendRequests();

    void ClearPendingNotify();
private:

    ChatDatabase chatDB;

    SOCKET sock;

    std::string username;


    std::atomic<bool> running;

    std::thread recvThread;


   std::queue<Message> replyQueue;


    // 互斥锁
    std::mutex replyMutex;

    // 条件变量
    std::condition_variable replyCV;

    std::atomic<bool> chatting = false;

    std::thread inputThread;

    std::string currentChatFriend;

    std::mutex chatMutex;

public:

    bool WaitReply(
        MessageType type,
        Message& msg);

    bool IsConnected() const;

    void ReceiveLoop();

    std::string GetUsername() const;

    std::vector<ChatRecord> GetHistory(
        const std::string& friendName);

    std::function<void()> OnDisconnected;

    std::atomic<bool> manualDisconnect = false;


    void SendChatMessage(const std::string& text);


    void Logout();

};
