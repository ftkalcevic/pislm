/*
 * Based on https://raw.githubusercontent.com/certik/record/master/arecord.c, which is based on alsa-utils aplay.c
 */
#include "CAudio.h"

#include <alsa/asoundlib.h>
#include <libintl.h>
#include <sys/time.h>

#define error(args...) do {\
	fprintf(stderr, "%s: %s:%d: ", "", __FUNCTION__, __LINE__); \
	fprintf(stderr, ##args); \
	putc('\n', stderr); \
} while (0)
#define _(msgid) gettext (msgid)
#define gettext_noop(msgid) msgid
#define N_(msgid) gettext_noop (msgid)

#ifndef timersub
#define	timersub(a, b, result) \
do { \
	(result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
	(result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
	if ((result)->tv_usec < 0) { \
		--(result)->tv_sec; \
		(result)->tv_usec += 1000000; \
	} \
} while (0)
#endif


static snd_pcm_sframes_t (*readi_func)(snd_pcm_t *handle, void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*readn_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
static snd_pcm_sframes_t (*writen_func)(snd_pcm_t *handle, void **bufs, snd_pcm_uframes_t size);
static void capture();
static ssize_t pcm_read(u_char *data, size_t rcount);
static void suspend(void);
static void xrun(void);

#define FORMAT_DEFAULT		-1
#define FORMAT_RAW		0
#define FORMAT_VOC		1
#define FORMAT_WAVE		2
#define FORMAT_AU		3
static int file_type = FORMAT_RAW;
static int timelimit = 0;
static off64_t pbrec_count = LLONG_MAX;
static struct {
	snd_pcm_format_t format;
	unsigned int channels;
	unsigned int rate;
} hwparams, rhwparams;
static snd_pcm_t *handle = NULL;
static int interleaved = 1;
static int quiet_mode = 0;
static unsigned period_time = 0;
static unsigned buffer_time = 0;
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t buffer_frames = 0;
static snd_output_t *log;
static 	snd_pcm_uframes_t chunk_size = -1;
static int avail_min = -1;
static int start_delay = 0;
static int stop_delay = 0;
static int verbose = 1;
static size_t bits_per_sample, bits_per_frame;
static size_t chunk_bytes;
static u_char *audiobuf = NULL;
static volatile int capture_stop = 0;
static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

#define SAMPLE_MODE_STEREO		1
#define SAMPLE_FORMAT			SND_PCM_FORMAT_S32_LE
#define SAMPLE_RATE				48000
#define SAMPLE_PERIOD			0.1
#define SAMPLE_BUFFER_SIZE		(SAMPLE_RATE * (SAMPLE_FORMAT==SND_PCM_FORMAT_S32_LE ? 4 : SAMPLE_FORMAT==SND_PCM_FORMAT_S24_LE ? 3 : 2) * (SAMPLE_MODE_STEREO==1?2:1) * SAMPLE_PERIOD)

CAudio::CAudio()
	: m_audioBuffer(2,SAMPLE_BUFFER_SIZE)
{
	// 1178  sudo arecord 
	//		-D plughw:2			device plughw:cardno
	//		-c2					channels
	//		-r 48000			sample rate
	//		-f S32_LE			format 
	//		-t wav				file type
	//		-V stereo			vu meter type
	//		-v					verbose
	//		file_stereo.wav		output file
	
	// 1189  arecord -D dmic_sv -c2 -r 48000 -f S32_LE -t wav -V mono -v recording.wav
	
	
}

int CAudio::init()
{
	const char *pcm_name = "plughw:2";
	//const char *pcm_name = "dmic_sv";
	int tmp, err;
	snd_pcm_info_t *info;

	timelimit = 0;	// 1 sec
	pbrec_count = LLONG_MAX;
	file_type = FORMAT_WAVE;
	memset(&rhwparams, 0, sizeof(rhwparams));
	handle = NULL;
	interleaved = 1;
	quiet_mode = 0;
	period_time = 0;
	buffer_time = 0;
	period_frames = 0;
	buffer_frames = 0;
	log = NULL;
	avail_min = -1;
	start_delay = 0;
	stop_delay = 0;
	audiobuf = NULL;
	capture_stop = 0;
	
	snd_pcm_info_alloca(&info);

	err = snd_output_stdio_attach(&log, stderr, 0);
	assert(err >= 0);

	stream = SND_PCM_STREAM_CAPTURE;
	start_delay  = 1;
	

	chunk_size = -1;
	rhwparams.format = SND_PCM_FORMAT_S32_LE;
	rhwparams.rate = 48000;
	rhwparams.channels = 2;
	int open_mode = 0;
	int nonblock = 0;
	
	err = snd_pcm_open(&handle, pcm_name, stream, open_mode);
	if (err < 0) {
		error(_("audio open error: %s"), snd_strerror(err));
		return 1;
	}

	if ((err = snd_pcm_info(handle, info)) < 0) {
		error(_("info error: %s"), snd_strerror(err));
		return 1;
	}

	if (nonblock) {
		err = snd_pcm_nonblock(handle, 1);
		if (err < 0) {
			error(_("nonblock setting error: %s"), snd_strerror(err));
			return 1;
		}
	}

	chunk_size = 1024;
	hwparams = rhwparams;

	audiobuf = (u_char *)malloc(1024);
	if (audiobuf == NULL) {
		error(_("not enough memory"));
		return 1;
	}

	writei_func = snd_pcm_writei;
	readi_func = snd_pcm_readi;
	writen_func = snd_pcm_writen;
	readn_func = snd_pcm_readn;


	//signal(SIGINT, signal_handler);
	//signal(SIGTERM, signal_handler);
	//signal(SIGABRT, signal_handler);
    capture();

	if (handle) {
		snd_pcm_close(handle);
		handle = NULL;
	}
	//snd_pcm_close(handle);
	//free(audiobuf);
	//snd_output_close(log);
	//snd_config_update_free_global();
	return EXIT_SUCCESS;

}

/* calculate the data count to read from/to dsp */
static off64_t calc_count(void)
{
	off64_t count;

	if (timelimit == 0) {
		count = pbrec_count;
	}
	else {
		count = snd_pcm_format_size(hwparams.format, hwparams.rate * hwparams.channels);
		count *= (off64_t)timelimit;
	}
	return count < pbrec_count ? count : pbrec_count;
}

static void set_params(void)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_uframes_t buffer_size;
	int err;
	size_t n;
	unsigned int rate;
	snd_pcm_uframes_t start_threshold, stop_threshold;
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(handle, params);
	if (err < 0) {
		error(_("Broken configuration for this PCM: no configurations available"));
		exit(EXIT_FAILURE);
	}
	else if (interleaved)
		err = snd_pcm_hw_params_set_access(handle,
			params,
			SND_PCM_ACCESS_RW_INTERLEAVED);
	else
		err = snd_pcm_hw_params_set_access(handle,
			params,
			SND_PCM_ACCESS_RW_NONINTERLEAVED);
	if (err < 0) {
		error(_("Access type not available"));
		exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
	if (err < 0) {
		error(_("Sample format non available"));
		exit(EXIT_FAILURE);
	}
	err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
	if (err < 0) {
		error(_("Channels count non available"));
		exit(EXIT_FAILURE);
	}

#if 0
	err = snd_pcm_hw_params_set_periods_min(handle, params, 2);
	assert(err >= 0);
#endif
	rate = hwparams.rate;
	err = snd_pcm_hw_params_set_rate_near(handle, params, &hwparams.rate, 0);
	assert(err >= 0);
	if ((float)rate * 1.05 < hwparams.rate || (float)rate * 0.95 > hwparams.rate) {
		if (!quiet_mode) {
			char plugex[64];
			const char *pcmname = snd_pcm_name(handle);
			fprintf(stderr, _("Warning: rate is not accurate (requested = %iHz, got = %iHz)\n"), rate, hwparams.rate);
			if (!pcmname || strchr(snd_pcm_name(handle), ':'))
				*plugex = 0;
			else
				snprintf(plugex,
					sizeof(plugex),
					"(-Dplug:%s)",
					snd_pcm_name(handle));
			fprintf(stderr,
				_("         please, try the plug plugin %s\n"),
				plugex);
		}
	}
	rate = hwparams.rate;
	if (buffer_time == 0 && buffer_frames == 0) {
		err = snd_pcm_hw_params_get_buffer_time_max(params,
			&buffer_time,
			0);
		assert(err >= 0);
		if (buffer_time > 500000)
			buffer_time = 500000;
	}
	if (period_time == 0 && period_frames == 0) {
		if (buffer_time > 0)
			period_time = buffer_time / 4;
		else
			period_frames = buffer_frames / 4;
	}
	if (period_time > 0)
		err = snd_pcm_hw_params_set_period_time_near(handle,
			params,
			&period_time,
			0);
	else
		err = snd_pcm_hw_params_set_period_size_near(handle,
			params,
			&period_frames,
			0);
	assert(err >= 0);
	if (buffer_time > 0) {
		err = snd_pcm_hw_params_set_buffer_time_near(handle,
			params,
			&buffer_time,
			0);
	}
	else {
		err = snd_pcm_hw_params_set_buffer_size_near(handle,
			params,
			&buffer_frames);
	}
	assert(err >= 0);
	err = snd_pcm_hw_params(handle, params);
	if (err < 0) {
		error(_("Unable to install hw params:"));
		snd_pcm_hw_params_dump(params, log);
		exit(EXIT_FAILURE);
	}
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
	if (chunk_size == buffer_size) {
		error(_("Can't use period equal to buffer size (%lu == %lu)"),
			chunk_size,
			buffer_size);
		exit(EXIT_FAILURE);
	}
	snd_pcm_sw_params_current(handle, swparams);
	if (avail_min < 0)
		n = chunk_size;
	else
		n = (double) rate * avail_min / 1000000;
	err = snd_pcm_sw_params_set_avail_min(handle, swparams, n);

	/* round up to closest transfer boundary */
	n = buffer_size;
	if (start_delay <= 0) {
		start_threshold = n + (double) rate * start_delay / 1000000;
	}
	else
		start_threshold = (double) rate * start_delay / 1000000;
	if (start_threshold < 1)
		start_threshold = 1;
	if (start_threshold > n)
		start_threshold = n;
	err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_threshold);
	assert(err >= 0);
	if (stop_delay <= 0) 
		stop_threshold = buffer_size + (double) rate * stop_delay / 1000000;
	else
		stop_threshold = (double) rate * stop_delay / 1000000;
	err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, stop_threshold);
	assert(err >= 0);

	if (snd_pcm_sw_params(handle, swparams) < 0) {
		error(_("unable to install sw params:"));
		snd_pcm_sw_params_dump(swparams, log);
		exit(EXIT_FAILURE);
	}

	if (verbose)
		snd_pcm_dump(handle, log);

	bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
	bits_per_frame = bits_per_sample * hwparams.channels;
	chunk_bytes = chunk_size * bits_per_frame / 8;
	audiobuf = (u_char *)realloc(audiobuf, chunk_bytes);
	if (audiobuf == NULL) {
		error(_("not enough memory"));
		exit(EXIT_FAILURE);
	}
	// fprintf(stderr, "real chunk_size = %i, frags = %i, total = %i\n", chunk_size, setup.buf.block.frags, setup.buf.block.frags * chunk_size);

	/* stereo VU-meter isn't always available... */
	//if(vumeter == VUMETER_STEREO) {
	//	if (hwparams.channels != 2 || !interleaved || verbose > 2)
	//		vumeter = VUMETER_MONO;
	//}

	buffer_frames = buffer_size; /* for position test */
}


static void capture()
{
	int tostdout = 0; /* boolean which describes output stream */
	int filecount = 0; /* number of files written */
	off64_t count, rest; /* number of bytes to capture */

	/* get number of bytes to capture */
	count = calc_count();
	if (count == 0)
		count = LLONG_MAX;
	/* WAVE-file should be even (I'm not sure), but wasting one byte
	   isn't a problem (this can only be in 8 bit mono) */
	if (count < LLONG_MAX)
		count += count % 2;
	else
		count -= count % 2;

	//printf("arecord: Recording audio to: %s\n", name);
	/* setup sound hardware */
	set_params();

	do {
		rest = count;
		
		/* capture */
		while (rest > 0 && capture_stop == 0) {
			size_t c = (rest <= (off64_t)chunk_bytes) ?
				(size_t)rest : chunk_bytes;
			size_t f = c * 8 / bits_per_frame;
			if (pcm_read(audiobuf, f) != f)
				break;
//			if (write(fd, audiobuf, c) != c) {
//				perror(name);
//				exit(EXIT_FAILURE);
//			}
			count -= c;
			rest -= c;
capture_stop = 1;
		}

		/* repeat the loop when format is raw without timelimit or
		 * requested counts of data are recorded
		 */
	} while (((file_type == FORMAT_RAW && !timelimit) || count > 0) &&
        capture_stop == 0);
	printf("arecord: Stopping capturing audio.\n");
}


/*
 *  read function
 */

static ssize_t pcm_read(u_char *data, size_t rcount)
{
	ssize_t r;
	size_t result = 0;
	size_t count = rcount;

	if (count != chunk_size) {
		count = chunk_size;
	}

	while (count > 0) {
		r = readi_func(handle, data, count);
		if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
			snd_pcm_wait(handle, 1000);
		}
		else if (r == -EPIPE) {
			xrun();
		}
		else if (r == -ESTRPIPE) {
			suspend();
		}
		else if (r < 0) {
			error(_("read error: %s"), snd_strerror(r));
			exit(EXIT_FAILURE);
		}
		if (r > 0) {
			//if (vumeter)
			//	compute_max_peak(data, r * hwparams.channels);
			result += r;
			count -= r;
			data += r * bits_per_frame / 8;
		}
	}
	return rcount;
}

/* I/O error handler */
static void xrun(void)
{
	snd_pcm_status_t *status;
	int res;
	
	snd_pcm_status_alloca(&status);
	if ((res = snd_pcm_status(handle, status)) < 0) {
		error(_("status error: %s"), snd_strerror(res));
		exit(EXIT_FAILURE);
	}
	if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
		struct timeval now, diff, tstamp;
		gettimeofday(&now, 0);
		snd_pcm_status_get_trigger_tstamp(status, &tstamp);
		timersub(&now, &tstamp, &diff);
		fprintf(stderr,
			_("%s!!! (at least %.3f ms long)\n"),
			stream == SND_PCM_STREAM_PLAYBACK ? _("underrun") : _("overrun"),
			diff.tv_sec * 1000 + diff.tv_usec / 1000.0);
		if (verbose) {
			fprintf(stderr, _("Status:\n"));
			snd_pcm_status_dump(status, log);
		}
		if ((res = snd_pcm_prepare(handle)) < 0) {
			error(_("xrun: prepare error: %s"), snd_strerror(res));
			exit(EXIT_FAILURE);
		}
		return;		/* ok, data should be accepted again */
	} if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
		if (verbose) {
			fprintf(stderr, _("Status(DRAINING):\n"));
			snd_pcm_status_dump(status, log);
		}
		if (stream == SND_PCM_STREAM_CAPTURE) {
			fprintf(stderr, _("capture stream format change? attempting recover...\n"));
			if ((res = snd_pcm_prepare(handle)) < 0) {
				error(_("xrun(DRAINING): prepare error: %s"), snd_strerror(res));
				exit(EXIT_FAILURE);
			}
			return;
		}
	}
	if (verbose) {
		fprintf(stderr, _("Status(R/W):\n"));
		snd_pcm_status_dump(status, log);
	}
	error(_("read/write error, state = %s"), snd_pcm_state_name(snd_pcm_status_get_state(status)));
	exit(EXIT_FAILURE);
}

/* I/O suspend handler */
static void suspend(void)
{
	int res;

	if (!quiet_mode)
		fprintf(stderr, _("Suspended. Trying resume. ")); fflush(stderr);
	while ((res = snd_pcm_resume(handle)) == -EAGAIN)
		sleep(1); /* wait until suspend flag is released */
	if (res < 0) {
		if (!quiet_mode)
			fprintf(stderr, _("Failed. Restarting stream. ")); fflush(stderr);
		if ((res = snd_pcm_prepare(handle)) < 0) {
			error(_("suspend: prepare error: %s"), snd_strerror(res));
			exit(EXIT_FAILURE);
		}
	}
	if (!quiet_mode)
		fprintf(stderr, _("Done.\n"));
}


/*
 
Is this running all the time?  
	Why not?
		Uses resources.
	Why?
		Coding easier.
Exposes double buffer.
	Need to lock buffer when in use.
Events when next buffer ready.

TODO
	* call alsa methods to determine hw capabilities - native bit rate.  max rate.
 */