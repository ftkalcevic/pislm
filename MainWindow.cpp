#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&m_audio, SIGNAL(BufferUpdate(unsigned char*, int)), this, SLOT(BufferUpdate(unsigned char*, int)));
	m_audio.init();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::btnStopClicked()
{
    m_audio.stop();
}

void MainWindow::ButtonClicked()
{
	QMessageBox msgBox(this);
    msgBox.setText("Hello, World!");
    msgBox.setWindowTitle("VisualGDB Qt Demo");
    msgBox.exec();
}

//bool MainWindow::event(QEvent *event)
//{
//	fprintf(stderr, "Event %d\n", event->type());
//	return QMainWindow::event(event);
//}


void MainWindow::BufferUpdate(unsigned char* buff, int len)
{
    ui->waveWidget->UpdateWave(buff, len);
    ui->fftWidget->UpdateWave(buff, len);
}

