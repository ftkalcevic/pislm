#pragma once

#include <QObject>
#include "CAudioBuffer.h"

class CAudio : public QObject
{
	Q_OBJECT
public :

	CAudio();
	int init();
	
private:
	CAudioBuffer m_audioBuffer;
};

