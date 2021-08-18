#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    
    return a.exec();
}


//// In lieu of not being able to get rotating touchscreen to work. (Screen, yes.  Touchscreen no).
//
//#include <QGraphicsScene>
//#include <QGraphicsView>
//
//int main(int argc, char *argv[])
//{
//	QApplication a(argc, argv);
//	MainWindow w;
//
//	QGraphicsScene *scene = new QGraphicsScene();
//	QGraphicsView *view = new QGraphicsView(scene);
//	QRect r(0, 0, 800, 480);
//	view->setGeometry(r);
//	view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//	view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//	scene->addWidget(&w);
//	//view->setScene(scene);
//	view->rotate(90);
//	view->show();
//
//	//w.show();
//
//	return a.exec();
//}