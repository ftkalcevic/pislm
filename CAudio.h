#pragma once

#include <QObject>
#include <QThread>
#include "CAudioBuffer.h"

class CAudio : public QThread
{
	Q_OBJECT

	void run() override;
public :

	CAudio();
	int init();
	void stop();

signals:
	void BufferUpdate(unsigned char* buff, int len);

public slots:
	void capture();

private:
	CAudioBuffer m_audioBuffer;

	void set_params(int frames_per_period);
};

