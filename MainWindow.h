#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "CAudio.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
		
protected slots:
    void ButtonClicked();
	
	virtual bool event(QEvent *event);
		
private:
    Ui::MainWindow *ui;
	
	CAudio m_audio;
};

#endif // MAINWINDOW_H
