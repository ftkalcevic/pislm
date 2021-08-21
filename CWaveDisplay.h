#pragma once

#include <QWidget>
#include <QVector>
#include <QLine>

class CWaveDisplay :
    public QWidget
{
    Q_OBJECT

public:
    CWaveDisplay(QWidget* parent = nullptr);
    void UpdateWave(u_int8_t* buff, int len);

protected:
    virtual void paintEvent(QPaintEvent* event);

    QVector<QVector<QPointF> > m_waves;
    qreal sample_offset[2];
    int sample_count[2];
    int sample_index;
};

