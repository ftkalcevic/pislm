#include "CFFTDisplay.h"
#include <QPainter>
#include <QFileInfo>

#define SAMPLE_SIZE	4800
const char* const filename = "/home/frankt/fftw.plan";

CFFTDisplay::CFFTDisplay(QWidget* parent)
	: QWidget(parent) 
	, in(NULL)
	, out(NULL)
{
	m_waves.resize(2);	// 2 channels
	for (int ch = 0; ch < 2; ch++)
		m_waves[ch].resize(SAMPLE_SIZE-1);
	InitFFTW();
}

CFFTDisplay::~CFFTDisplay()
{
	fftw_destroy_plan(plan);
	if ( in != NULL )
		fftw_free(in); 
	if (out != NULL)
		fftw_free(out);
}


void CFFTDisplay::paintEvent(QPaintEvent* event)
{
	QPainter p(this);

	QColor pen(200, 200, 200);
	p.setPen(pen);
	p.drawLine(0, 0, width() - 1, 0);
	p.drawLine(width() - 1, 0, width() - 1, height() - 1);
	p.drawLine(width() - 1, height() - 1, 0, height() - 1);
	p.drawLine(0, height() - 1, 0, 0);
	p.drawLine(0, 0, width() - 1, height() - 1);
	p.drawLine(width() - 1, 0, 0, height() - 1);

	p.drawLine(0, height() / 2, width() - 1, height() / 2);

	if (m_waves.count() > 0 && m_waves[0].count() > 0)
	{
		QColor penL(255, 0, 0);
		p.setPen(penL);
		p.drawLines(m_waves[0]);
		QColor penR(0, 0, 255);
		p.setPen(penR);
		p.drawLines(m_waves[1]);
	}
}


void CFFTDisplay::InitFFTW()
{
	if (in != NULL)
		return;

	in = (double*)fftw_malloc(sizeof(double) * SAMPLE_SIZE);
	out = (double*)fftw_malloc(sizeof(double) * SAMPLE_SIZE);

	bool loaded = false;
	if (QFileInfo::exists(filename))
	{
		if (fftw_import_wisdom_from_filename(filename))
			loaded = true;
	}

	plan = fftw_plan_r2r_1d(SAMPLE_SIZE, in, out, FFTW_REDFT10, FFTW_EXHAUSTIVE);	// FFTW_EXHAUSTIVE
	if ( !loaded )
		fftw_export_wisdom_to_filename(filename);
}



void CFFTDisplay::UpdateWave(u_int8_t* buff, int len)
{
	int count = len / 4 / 2;

	for (int ch = 0; ch < 2; ch++)
	{
		int32_t* data_ptr = ((int32_t*)buff) + ch;
		double* in_ptr = in;

		for (int i = 0; i < count; i++, data_ptr += 2, in_ptr++ )
		{
			*in_ptr = *data_ptr >> 14;
		}
		fftw_execute(plan);

		double* out_ptr = out;
		double last_x, last_y;
		for (int i = 0; i < count; i++)
		{
			qreal sample = *out_ptr++;
			sample /= SAMPLE_SIZE;
			qreal x = (qreal)width() * (qreal)i / (qreal)SAMPLE_SIZE;
			qreal y = (qreal)height() - 1 - (qreal)height() * sample / (qreal)(1 << 3);

			if (i != 0)
			{
				m_waves[ch][i - 1].setLine( last_x, last_y, x, y );
			}
			last_x = x;
			last_y = y;
		}
	}

	update();
}
