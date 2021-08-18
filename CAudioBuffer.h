#pragma once

#include <QObject>
#include <QVector>

class CAudioBuffer :
    public QObject
{
	Q_OBJECT
public :
	CAudioBuffer(int buffers_count, int buffer_size)	
		: m_buffers(buffers_count)
	{
		for (int i = 0; i < m_buffers.count(); i++)
			m_buffers[i].fill(0, buffer_size);
		m_activeBuffer = 0;
	}
	
	QVector<QVector<u_int32_t> > m_buffers;
	int m_activeBuffer;
};

