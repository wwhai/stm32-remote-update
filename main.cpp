#include "serverwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::addLibraryPath(".");
    QApplication a(argc, argv);
    ServerWindow w;

    w.setWindowOpacity(1);
    w.setWindowFlags(Qt::FramelessWindowHint);
    w.show();

    QFile f(":qdarkstyle/style.qss");
    if (!f.exists()) {
        printf("Unable to set stylesheet, file not found\n");
    } else {
        f.open(QFile::ReadOnly | QFile::Text);
        QTextStream ts(&f);
        qApp->setStyleSheet(ts.readAll());
    }

    return a.exec();
}
