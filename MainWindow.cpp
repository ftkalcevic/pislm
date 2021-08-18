#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ButtonClicked()
{
	m_audio.init();
	QMessageBox msgBox(this);
    msgBox.setText("Hello, World!");
    msgBox.setWindowTitle("VisualGDB Qt Demo");
    msgBox.exec();
}

bool MainWindow::event(QEvent *event)
{
	fprintf(stderr, "Event %d\n", event->type());
	return QMainWindow::event(event);
}
