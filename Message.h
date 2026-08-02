#pragma once

#include <cstring>

constexpr int MAX_NAME_LEN = 32;
constexpr int MAX_PASSWORD_LEN = 64;
constexpr int MAX_TEXT_LEN = 512;

//============================
// 消息类型
//============================
enum class MessageType
{
    LOGIN,
    LOGIN_RESULT,

    REGISTER,
    REGISTER_RESULT,

    CHAT,
    CHAT_RESULT,

    READY,

    LOGOUT,

    ADD_FRIEND,
    ADD_FRIEND_RESULT,

    FRIEND_REQUEST_LIST,

    DELETE_FRIEND,

    ACCEPT_FRIEND,

    REJECT_FRIEND,

    FRIEND_LIST,

    PENDING_NOTIFY

};

//============================
// 消息结构
//============================
struct Message
{
    MessageType type;

    int result;                 // 0失败 1成功

    int count;

    char sender[MAX_NAME_LEN];

    char receiver[MAX_NAME_LEN];

    char password[MAX_PASSWORD_LEN];

    char text[MAX_TEXT_LEN];
    
    Message()
    {
        memset(this, 0, sizeof(Message));
    }
};