#include <QtWidgets>
#include "ChatDatabase.h"
#include <sstream>
#include <iostream>

using namespace std;

ChatDatabase::ChatDatabase()
{
    db = nullptr;
}

ChatDatabase::~ChatDatabase()
{
    Close();
}

bool ChatDatabase::Open(const std::string& dbName)
{
    if (sqlite3_open(dbName.c_str(), &db) != SQLITE_OK)
    {

        return false;
    }

    return true;
}

void ChatDatabase::Close()
{
    if (db != nullptr)
    {
        sqlite3_close(db);

        db = nullptr;
    }
}

bool ChatDatabase::CreateTable()
{
    const char* sql =
        "CREATE TABLE IF NOT EXISTS chat_history("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "receiver TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "send_time DATETIME DEFAULT (datetime('now','localtime'))"
        ");";

    const char* sql2 =
        "CREATE TABLE IF NOT EXISTS chat_session("
        "friend_name TEXT PRIMARY KEY,"
        "has_new INTEGER DEFAULT 0"
        ");";

    // 新增：好友申请提醒表
    const char* sql3 =
        "CREATE TABLE IF NOT EXISTS friend_session("
        "id INTEGER PRIMARY KEY,"
        "has_new INTEGER DEFAULT 0"
        ");";

    char* errMsg = nullptr;

    // chat_history
    int rc = sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {

        if (errMsg)
        {
            sqlite3_free(errMsg);
        }

        return false;
    }

    // chat_session
    rc = sqlite3_exec(
        db,
        sql2,
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {

        if (errMsg)
        {
            sqlite3_free(errMsg);
        }

        return false;
    }

    // friend_session
    rc = sqlite3_exec(
        db,
        sql3,
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {

        if (errMsg)
        {
            sqlite3_free(errMsg);
        }

        return false;
    }

    return true;
}

bool ChatDatabase::InsertMessage(
    const std::string& sender,
    const std::string& receiver,
    const std::string& content)
{
    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream ss;

    ss << "INSERT INTO chat_history("
        << "sender,receiver,content)"
        << " VALUES('"
        << sender
        << "','"
        << receiver
        << "','"
        << content
        << "');";

    char* errMsg = nullptr;

    int rc = sqlite3_exec(
        db,
        ss.str().c_str(),
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {

        if (errMsg)
        {
            sqlite3_free(errMsg);
        }

        return false;
    }

    return true;
}

std::vector<ChatRecord> ChatDatabase::GetHistory(
    const std::string& selfUser,
    const std::string& friendName)
{
    std::vector<ChatRecord> history;

    std::stringstream ss;

    ss << "SELECT sender,"
        << "receiver,"
        << "content,"
        << "send_time "
        << "FROM ("

        << "SELECT * FROM chat_history "
        << "WHERE "
        << "(sender='"
        << selfUser
        << "' AND receiver='"
        << friendName
        << "') "
        << "OR "
        << "(sender='"
        << friendName
        << "' AND receiver='"
        << selfUser
        << "') "

        << "ORDER BY id DESC "
        << "LIMIT 50"

        << ") "

        << "ORDER BY id ASC;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
        db,
        ss.str().c_str(),
        -1,
        &stmt,
        nullptr) != SQLITE_OK)
    {
        return history;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        ChatRecord record;

        record.sender =
            (const char*)sqlite3_column_text(stmt, 0);

        record.receiver =
            (const char*)sqlite3_column_text(stmt, 1);

        record.content =
            (const char*)sqlite3_column_text(stmt, 2);

        record.time =
            (const char*)sqlite3_column_text(stmt, 3);

        history.push_back(record);
    }
    sqlite3_finalize(stmt);

    return history;
}

std::vector<RecentChat> ChatDatabase::GetRecentChats(
    const std::string& selfUser)
{
 
    std::vector<RecentChat> recent;

    const char* sql =
        "SELECT sender,receiver,content,send_time "
        "FROM chat_history "
        "ORDER BY id DESC;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        nullptr) != SQLITE_OK)
    {
        return recent;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        std::string sender =
            (const char*)sqlite3_column_text(stmt, 0);

        std::string receiver =
            (const char*)sqlite3_column_text(stmt, 1);

        std::string content =
            (const char*)sqlite3_column_text(stmt, 2);

        std::string time =
            (const char*)sqlite3_column_text(stmt, 3);

        std::string friendName;

        if (sender == selfUser)
            friendName = receiver;
        else if (receiver == selfUser)
            friendName = sender;
        else
            continue;

        bool exist = false;

        for (auto& item : recent)
        {
            if (item.friendName == friendName)
            {
                exist = true;
                break;
            }
        }

        if (exist)
            continue;

        RecentChat chat;

        chat.friendName = friendName;

        chat.lastMessage = content;

        chat.lastTime = time;

        recent.push_back(chat);
    }

    sqlite3_finalize(stmt);

    return recent;
}

bool ChatDatabase::SetNewMessage(
    const std::string& friendName)
{
    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream ss;

    ss << "SELECT friend_name "
        << "FROM chat_session "
        << "WHERE friend_name='"
        << friendName
        << "';";

    sqlite3_stmt* stmt = nullptr;

    bool exist = false;

    if (sqlite3_prepare_v2(
        db,
        ss.str().c_str(),
        -1,
        &stmt,
        nullptr) == SQLITE_OK)
    {
        exist =
            (sqlite3_step(stmt) == SQLITE_ROW);

        sqlite3_finalize(stmt);
    }

    ss.str("");
    ss.clear();

    if (exist)
    {
        ss << "UPDATE chat_session "
            << "SET has_new=1 "
            << "WHERE friend_name='"
            << friendName
            << "';";
    }
    else
    {
        ss << "INSERT INTO chat_session("
            << "friend_name,has_new)"
            << " VALUES('"
            << friendName
            << "',1);";
    }

    char* errMsg = nullptr;

    return sqlite3_exec(
        db,
        ss.str().c_str(),
        nullptr,
        nullptr,
        &errMsg) == SQLITE_OK;
}

bool ChatDatabase::HasNewMessage(
    const std::string& friendName)
{
    std::stringstream ss;

    ss << "SELECT has_new "
        << "FROM chat_session "
        << "WHERE friend_name='"
        << friendName
        << "';";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
        db,
        ss.str().c_str(),
        -1,
        &stmt,
        nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool result = false;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result =
            sqlite3_column_int(stmt, 0) == 1;
    }

    sqlite3_finalize(stmt);

    return result;
}

bool ChatDatabase::ClearNewMessage(
    const std::string& friendName)
{

    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream ss;

    ss << "UPDATE chat_session "
        << "SET has_new=0 "
        << "WHERE friend_name='"
        << friendName
        << "';";

    char* errMsg = nullptr;

    return sqlite3_exec(
        db,
        ss.str().c_str(),
        nullptr,
        nullptr,
        &errMsg) == SQLITE_OK;
}

bool ChatDatabase::DeleteHistory(
    const std::string& selfUser,
    const std::string& friendName)
{
    std::lock_guard<std::mutex> lock(mtx);

    std::stringstream ss;

    ss << "DELETE FROM chat_history "
        << "WHERE "
        << "(sender='"
        << selfUser
        << "' AND receiver='"
        << friendName
        << "') "
        << "OR "
        << "(sender='"
        << friendName
        << "' AND receiver='"
        << selfUser
        << "');";

    char* errMsg = nullptr;

    int rc = sqlite3_exec(
        db,
        ss.str().c_str(),
        nullptr,
        nullptr,
        &errMsg);

    if (rc != SQLITE_OK)
    {
        if (errMsg)
        {
            std::cout << errMsg << std::endl;
            sqlite3_free(errMsg);
        }

        return false;
    }

    return true;
}

bool ChatDatabase::HasAnyNewMessage()
{

    const char* sql =
        "SELECT 1 "
        "FROM chat_session "
        "WHERE has_new=1 "
        "LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool result =
        (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);

    return result;
}
bool ChatDatabase::SetPendingNotify()
{
    std::lock_guard<std::mutex> lock(mtx);

    const char* sql =
        "INSERT OR REPLACE INTO friend_session "
        "(id,has_new) VALUES(1,1);";

    char* errMsg = nullptr;

    return sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errMsg) == SQLITE_OK;
}
bool ChatDatabase::HasPendingNotify()
{

    const char* sql =
        "SELECT has_new "
        "FROM friend_session "
        "WHERE id=1;";

    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(
        db,
        sql,
        -1,
        &stmt,
        nullptr) != SQLITE_OK)
    {
        return false;
    }

    bool result = false;

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result =
            sqlite3_column_int(stmt, 0) == 1;
    }

    sqlite3_finalize(stmt);

    return result;
}
bool ChatDatabase::ClearPendingNotify()
{
    std::lock_guard<std::mutex> lock(mtx);

    const char* sql =
        "UPDATE friend_session "
        "SET has_new=0 "
        "WHERE id=1;";

    char* errMsg = nullptr;

    return sqlite3_exec(
        db,
        sql,
        nullptr,
        nullptr,
        &errMsg) == SQLITE_OK;
}