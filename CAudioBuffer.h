#pragma once

#include <QObject>
#include <QVector>

class CAudioBuffer :
    public QObject
{
	Q_OBJECT
public :
	CAudioBuffer(int buffers_count)	
		: m_buffers(buffers_count)
	{
		m_activeBuffer = 0;
	}

	void resize(int buffer_size)
	{
		for (int i = 0; i < m_buffers.count(); i++)
			m_buffers[i].resize(buffer_size);
	}

	int size()
	{
		return m_buffers[0].count();
	}
	
	u_char *buffer(unsigned int id)
	{
		Q_ASSERT(id < m_buffers.count());
		return m_buffers[id].data();
	}
	
	QVector<QVector<u_int8_t> > m_buffers;
	int m_activeBuffer;
};

