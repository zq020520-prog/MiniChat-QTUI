#include <QtWidgets>
#include "MiniChat.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFont font("Microsoft YaHei", 16);

    app.setFont(font);

    //这里输入自己的服务器IP
    QString serverIP ="192.168.43.128";

    MiniChat window(serverIP);

    window.show();
    return app.exec();
}
