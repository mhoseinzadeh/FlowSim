#include "mainwindow.h"
#include <QApplication>
#include <QtCore>

#include <cstdlib>     /* srand, rand */
#include <ctime>       /* time */
int main(int argc, char *argv[])
{
    srand(time(NULL));
    QApplication::setStyle("Fusion");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
