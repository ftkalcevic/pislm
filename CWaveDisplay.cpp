#include "CWaveDisplay.h"

#include <QPainter>

#define SAMPLES_PER_BLOCK	(4800)
#define BLOCKS				(1)
#define SAMPLES				(SAMPLES_PER_BLOCK*BLOCKS)
#define RAW_SAMPLE_MAX		(1000)

CWaveDisplay::CWaveDisplay(QWidget* parent)
	: QWidget( parent )
{
	for (int ch = 0; ch < 2; ch++)
	{
		sample_offset[ch] = 0;
		sample_count[ch] = 0;
	}
	sample_index = 0;
}


void CWaveDisplay::paintEvent(QPaintEvent* event)
{
	QPainter p(this);

	QColor pen(200, 200, 200);
	p.setPen(pen);
	p.drawLine(0, 0, width() - 1, 0);
	p.drawLine(width() - 1, 0, width()-1, height() - 1);
	p.drawLine(width() - 1, height() - 1, 0, height() - 1);
	p.drawLine(0, height() - 1, 0, 0);
	p.drawLine(0, 0, width() - 1, height() - 1);
	p.drawLine(width() - 1, 0, 0, height() - 1);

	p.drawLine(0, height()/2, width()-1, height()/2);

	if (m_waves.count() > 0 && m_waves[0].count() > 0)
	{
		QColor penL(255, 0, 0);
		p.setPen(penL);
		p.drawPolyline(m_waves[0].data(), m_waves[0].count() );
		QPointF* d = m_waves[0].data();
		QColor penR(0, 0, 255);
		p.setPen(penR);
		p.drawPolyline(m_waves[1].data(), m_waves[1].count());
	}
}


void CWaveDisplay::UpdateWave(u_int8_t* buff, int len)
{
	if (m_waves.count() != 2)
	{
		m_waves.resize(2);	// 2 channels
		for (int ch = 0; ch < 2; ch++)
		{
			m_waves[ch].resize(SAMPLES);
			for (int i = 0; i < m_waves[ch].count(); i++)
			{
				m_waves[ch][i].setX((qreal)width() * (qreal)i / (qreal)(SAMPLES));
				m_waves[ch][i].setY(0);
			}
		}
	}

	qreal sample_sum[2] = { 0 };

	//if (sample_index >= SAMPLES)
	//	return;

	int count = len / 4 / 2;
	//sample_count += count;
	//for (int ch = 0; ch < 2; ch++)
	//	m_waves[ch].resize(sample_count);

	//u_int8_t buf[len];
	//memcpy(buf, buff, len);
	//int32_t* data = (int32_t*)buf;

	uint32_t* data = (uint32_t*)buff;
	for ( int i = 0; i < count; i++, sample_index++ )
		for (int ch = 0; ch < 2; ch++)
		{
			int32_t s = *data++;
			s >>= 14;
			qreal sample = s;
			//qreal x = (qreal)width() * (sample_index - 1) / (qreal)SAMPLES;
			qreal y = (qreal)height() / 2 + (qreal)(height()/2) * (sample-sample_offset[ch]) / (qreal)(RAW_SAMPLE_MAX);

			m_waves[ch][sample_index].setY(y);

			sample_sum[ch] += sample;
		}
	if (sample_index >= SAMPLES)
		sample_index = 0;

	for ( int ch = 0; ch < 2; ch++ )
		sample_offset[ch] = sample_sum[ch] / count;
	update();
}
