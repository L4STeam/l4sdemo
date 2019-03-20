#include "mainwindow.h"

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStyleFactory>

int main( int argc, char **argv )
{
    QApplication a( argc, argv );
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QPalette p = QApplication::palette();
    p.setColor(QPalette::Background, Qt::white);
    QApplication::setPalette(p);
    MainWindow mainWindow;
    mainWindow.show();
    return a.exec();
}
