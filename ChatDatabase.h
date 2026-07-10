#pragma once
#include <QtWidgets>
#include <string>
#include <vector>
#include "sqlite3.h"

struct ChatRecord
{
    std::string sender;

    std::string receiver;

    std::string content;

    std::string time;
};
struct RecentChat
{
    std::string friendName;

    std::string lastMessage;

    std::string lastTime;

    int unread = 0;
};
class ChatDatabase
{
public:

    ChatDatabase();

    ~ChatDatabase();

    bool Open(const std::string& dbName);

    void Close();

    bool CreateTable();

    bool SetPendingNotify();

    bool HasPendingNotify();

    bool ClearPendingNotify();

    bool DeleteHistory(
        const std::string& selfUser,
        const std::string& friendName);

    bool InsertMessage(
        const std::string& sender,
        const std::string& receiver,
        const std::string& content);

    std::vector<ChatRecord> GetHistory(
        const std::string& selfUser,
        const std::string& friendName);

    std::vector<RecentChat> GetRecentChats(
        const std::string& selfUser);

    bool SetNewMessage(
        const std::string& friendName);

    bool HasNewMessage(
        const std::string& friendName);

    bool HasAnyNewMessage();

    bool ClearNewMessage(
        const std::string& friendName);

private:

    sqlite3* db;
};
