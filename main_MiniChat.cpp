#include <QtWidgets>
#include "MiniChat.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFont font("Microsoft YaHei", 16);

    app.setFont(font);

    //输入自己的服务器IP
    QString serverIP ="127.0.0.1";

    MiniChat window(serverIP);

    window.show();
    return app.exec();
}
