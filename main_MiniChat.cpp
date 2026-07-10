#include <QtWidgets>
#include "MiniChat.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QFont font("Microsoft YaHei", 16);
    app.setFont(font);
    MiniChat window;
    window.show();
    return app.exec();
}
