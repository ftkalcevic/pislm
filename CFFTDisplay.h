#pragma once

#include <QWidget>
#include <QVector>
#include <QLine>
#include <fftw3.h>

class CFFTDisplay :
    public QWidget
{
    Q_OBJECT

public:
    CFFTDisplay(QWidget* parent = nullptr);
    ~CFFTDisplay();
    void UpdateWave(u_int8_t* buff, int len);

protected:
    virtual void paintEvent(QPaintEvent* event);
    void InitFFTW();

    double* in, * out;
    fftw_plan plan;
    QVector<QVector<QLineF> > m_waves;
};

