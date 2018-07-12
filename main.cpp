#include "mainwindow.h"
#include <QApplication>
#include <QSoundEffect>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QSoundEffect voice;
    voice.setSource(QUrl::fromLocalFile(":/sounds/voice.wav"));
    voice.setVolume(0.02f);
    voice.play();


    return a.exec();
}
