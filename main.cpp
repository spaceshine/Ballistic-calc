#include "mainwindow.h"

#include <QApplication>
#include <QtDataVisualization/Q3DScatter>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
