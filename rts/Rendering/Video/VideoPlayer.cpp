/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "VideoPlayer.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <limits>
#include <iterator>
#include <mutex>
#include <optional>
#include <thread>

#include "System/FileSystem/FileHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/Log/ILog.h"
#include "System/Platform/Threading.h"
#include "System/Sound/ISound.h"
#include "System/Sound/PCMStream.h"
#include "System/StringUtil.h"
#include "System/Threading/SpringThreading.h"

#include <tracy/Tracy.hpp>

#ifdef RECOIL_VIDEO
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}
#endif

namespace video {

CONFIG(int, VideoMaxActiveDecoders).defaultValue(4).minimumValue(1).maximumValue(16);
CONFIG(int, VideoMaxFileSizeMB).defaultValue(512).minimumValue(1).maximumValue(2047);
CONFIG(int, VideoMaxWidth).defaultValue(4096).minimumValue(16).maximumValue(16384);
CONFIG(int, VideoMaxHeight).defaultValue(2160).minimumValue(16).maximumValue(16384);
CONFIG(int, VideoQueuedFrames).defaultValue(4).minimumValue(2).maximumValue(16);
CONFIG(int, VideoProbeSizeMB).defaultValue(5).minimumValue(1).maximumValue(32);
CONFIG(int, VideoAnalyzeDurationMS).defaultValue(5000).minimumValue(100).maximumValue(30000);
CONFIG(int, VideoDecoderThreads).defaultValue(4).minimumValue(1).maximumValue(8);

namespace {
	std::atomic<int> activeDecoders = 0;

	std::int64_t NowUs()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	// Tracy plot names
	static const char* const videoFrameQueuePlot = "VideoFrameQueue";
	static const char* const videoPCMQueueBytesPlot = "VideoPCMQueueBytes";
	static const char* const videoDroppedFramesPlot = "VideoDroppedFrames";
	static const char* const videoPositionPlot = "VideoPositionUs";
	static const char* const videoActiveDecodersPlot = "VideoActiveDecoders";

	// Register plot formats once at startup
	struct TracyPlotRegistrar {
		TracyPlotRegistrar()
		{
			TracyPlotConfig(videoFrameQueuePlot, tracy::PlotFormatType::Number, true, false, tracy::Color::MediumAquamarine);
			TracyPlotConfig(videoPCMQueueBytesPlot, tracy::PlotFormatType::Memory, true, false, tracy::Color::MediumAquamarine);
			TracyPlotConfig(videoDroppedFramesPlot, tracy::PlotFormatType::Number, true, false, tracy::Color::OrangeRed);
			TracyPlotConfig(videoPositionPlot, tracy::PlotFormatType::Number, true, false, tracy::Color::CornflowerBlue);
			TracyPlotConfig(videoActiveDecodersPlot, tracy::PlotFormatType::Number, true, false, tracy::Color::Gold);
		}
	} tracyPlotRegistrar;
}

const char* ToString(PlaybackState state)
{
	switch (state) {
		case PlaybackState::Opening:  return "opening";
		case PlaybackState::Ready:    return "ready";
		case PlaybackState::Playing:  return "playing";
		case PlaybackState::Paused:   return "paused";
		case PlaybackState::Seeking:  return "seeking";
		case PlaybackState::Finished: return "finished";
		case PlaybackState::Stopped:  return "stopped";
		case PlaybackState::Error:    return "error";
	}
	return "error";
}

struct VideoPlayer::Impl {
	Impl(std::string path, std::string modes, VideoOptions options)
		: path(std::move(path))
		, modes(std::move(modes))
		, options(options)
	{
		maxQueuedFrames = static_cast<std::size_t>(configHandler->GetInt("VideoQueuedFrames"));
		maxFileSize = static_cast<std::int64_t>(configHandler->GetInt("VideoMaxFileSizeMB")) * 1024 * 1024;
		maxWidth = configHandler->GetInt("VideoMaxWidth");
		maxHeight = configHandler->GetInt("VideoMaxHeight");
		probeSize = static_cast<std::int64_t>(configHandler->GetInt("VideoProbeSizeMB")) * 1024 * 1024;
		analyzeDuration = static_cast<std::int64_t>(configHandler->GetInt("VideoAnalyzeDurationMS")) * 1000000 / 1000;
		decoderThreads = configHandler->GetInt("VideoDecoderThreads");
		if (activeDecoders.fetch_add(1) >= configHandler->GetInt("VideoMaxActiveDecoders")) {
			activeDecoders.fetch_sub(1);
			state = PlaybackState::Error;
			error = "maximum number of active video decoders reached";
			return;
		}
		countedDecoder = true;
		TracyPlot(videoActiveDecodersPlot, static_cast<int64_t>(activeDecoders));
#ifdef RECOIL_VIDEO
		if (this->options.audio && sound != nullptr && !sound->SoundThreadQuit())
			pcmStream = sound->CreatePCMStream();
#endif
		worker = spring::thread(&Impl::WorkerMain, this);
	}

	~Impl()
	{
		{
			std::lock_guard<std::mutex> lock(mutex);
			shutdown = true;
		}
		cv.notify_all();
		if (worker.joinable())
			worker.join();
		if (pcmStream != nullptr)
			pcmStream->Close();
		if (countedDecoder) {
			activeDecoders.fetch_sub(1);
			TracyPlot(videoActiveDecodersPlot, static_cast<int64_t>(activeDecoders));
		}
	}

	void SetError(const std::string& message)
	{
		std::lock_guard<std::mutex> lock(mutex);
		error = message;
		state = PlaybackState::Error;
		cv.notify_all();
		LOG_L(L_ERROR, "[VideoPlayer] %s: %s", path.c_str(), message.c_str());
	}

	std::int64_t PositionUsLocked() const
	{
		if (state == PlaybackState::Playing && pcmStream != nullptr && pcmStream->IsAudible() && !pcmStream->IsDrained())
			return std::max<std::int64_t>(0, pcmStream->GetPositionUs());
		if (state == PlaybackState::Playing)
			return std::max<std::int64_t>(0, clockBaseUs + (NowUs() - clockEpochUs));
		return std::max<std::int64_t>(0, clockBaseUs);
	}

	void StartClockLocked(std::int64_t positionUs)
	{
		clockBaseUs = std::max<std::int64_t>(0, positionUs);
		clockEpochUs = NowUs();
	}

	void WorkerMain();

	std::string path;
	std::string modes;
	VideoOptions options;
	mutable std::mutex mutex;
	std::condition_variable cv;
	spring::thread worker;
	std::deque<VideoFrame> frames;
	PlaybackState state = PlaybackState::Opening;
	std::string error;
	int width = 0;
	int height = 0;
	std::int64_t durationUs = 0;
	std::int64_t clockBaseUs = 0;
	std::int64_t clockEpochUs = 0;
	std::optional<std::int64_t> requestedSeekUs;
	PlaybackState seekTargetState = PlaybackState::Playing;
	std::uint64_t droppedFrames = 0;
	bool hasAudio = false;
	std::atomic<bool> shutdown = false;
	bool countedDecoder = false;
	std::size_t maxQueuedFrames = 4;
	std::int64_t maxFileSize = 512LL * 1024 * 1024;
	int maxWidth = 4096;
	int maxHeight = 2160;
	std::int64_t probeSize = 5 * 1024 * 1024;
	std::int64_t analyzeDuration = 5 * 1000000;
	int decoderThreads = 4;
	std::shared_ptr<IPCMStream> pcmStream;
};

#ifndef RECOIL_VIDEO

void VideoPlayer::Impl::WorkerMain()
{
	SetError("video playback is disabled in this build");
}

#else

namespace {
	std::string FFmpegError(int code)
	{
		char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
		av_strerror(code, buffer, sizeof(buffer));
		return buffer;
	}

	struct FormatDeleter {
		void operator()(AVFormatContext* context) const
		{
			if (context != nullptr)
				avformat_close_input(&context);
		}
	};

	struct CodecDeleter {
		void operator()(AVCodecContext* context) const { avcodec_free_context(&context); }
	};

	struct PacketDeleter {
		void operator()(AVPacket* packet) const { av_packet_free(&packet); }
	};

	struct FrameDeleter {
		void operator()(AVFrame* frame) const { av_frame_free(&frame); }
	};

	struct IODeleter {
		void operator()(AVIOContext* context) const { avio_context_free(&context); }
	};

	int ReadPacket(void* opaque, std::uint8_t* buffer, int size)
	{
		auto* file = static_cast<CFileHandler*>(opaque);
		const int read = file->Read(buffer, size);
		return (read == 0) ? AVERROR_EOF : read;
	}

	std::int64_t FFmpegSeekCB(void* opaque, std::int64_t offset, int whence)
	{
		auto* file = static_cast<CFileHandler*>(opaque);
		if ((whence & AVSEEK_SIZE) != 0)
			return file->FileSize();

		whence &= ~AVSEEK_FORCE;
		if (offset > std::numeric_limits<int>::max() || offset < -static_cast<std::int64_t>(std::numeric_limits<int>::max()))
			return AVERROR(EINVAL);
		std::int64_t base = 0;
		switch (whence) {
			case SEEK_SET: base = 0; break;
			case SEEK_CUR: base = file->GetPos(); break;
			case SEEK_END: base = file->FileSize(); break;
			default: return AVERROR(EINVAL);
		}

		const std::int64_t target = base + offset;
		if (target < 0 || target > file->FileSize() || target > std::numeric_limits<int>::max())
			return AVERROR(EINVAL);
		file->Seek(static_cast<int>(target), std::ios_base::beg);
		return file->GetPos();
	}
}

void VideoPlayer::Impl::WorkerMain()
{
	Threading::SetThreadName("video-decode");

	CFileHandler file(path, modes);
	const std::string extension = StringToLower(file.GetFileExt());
	if (extension != "mp4" && extension != "mov") {
		SetError("unsupported container; use an .mp4 or .mov file");
		return;
	}
	if (!file.FileExists()) {
		SetError("file not found in permitted VFS modes");
		return;
	}
	if (file.FileSize() <= 0 || file.FileSize() > maxFileSize) {
		SetError("video file is empty or exceeds VideoMaxFileSizeMB");
		return;
	}

	constexpr int ioBufferSize = 32 * 1024;
	auto* ioBuffer = static_cast<unsigned char*>(av_malloc(ioBufferSize));
	if (ioBuffer == nullptr) {
		SetError("could not allocate FFmpeg I/O buffer");
		return;
	}

	AVIOContext* io = avio_alloc_context(ioBuffer, ioBufferSize, 0, &file, ReadPacket, nullptr, FFmpegSeekCB);
	if (io == nullptr) {
		av_free(ioBuffer);
		SetError("could not create FFmpeg VFS adapter");
		return;
	}
	std::unique_ptr<AVIOContext, IODeleter> ioOwner(io);

	AVFormatContext* rawFormat = avformat_alloc_context();
	if (rawFormat == nullptr) {
		SetError("could not allocate FFmpeg format context");
		return;
	}
	rawFormat->pb = io;
	rawFormat->flags |= AVFMT_FLAG_CUSTOM_IO;
	rawFormat->probesize = probeSize;
	rawFormat->max_analyze_duration = analyzeDuration;
	rawFormat->interrupt_callback.callback = [](void* opaque) -> int {
		return static_cast<Impl*>(opaque)->shutdown ? 1 : 0;
	};
	rawFormat->interrupt_callback.opaque = this;

	const AVInputFormat* inputFormat = av_find_input_format("mov");
	if (inputFormat == nullptr) {
		avformat_free_context(rawFormat);
		SetError("FFmpeg MOV/MP4 demuxer is unavailable");
		return;
	}
	int result = avformat_open_input(&rawFormat, nullptr, inputFormat, nullptr);
	if (result < 0) {
		avformat_free_context(rawFormat);
		SetError("could not open MP4/MOV: " + FFmpegError(result));
		return;
	}
	std::unique_ptr<AVFormatContext, FormatDeleter> format(rawFormat);

	result = avformat_find_stream_info(format.get(), nullptr);
	if (result < 0) {
		SetError("could not read stream metadata: " + FFmpegError(result));
		return;
	}

	const int videoStream = av_find_best_stream(format.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (videoStream < 0) {
		SetError("no video stream found");
		return;
	}
	const int audioStream = av_find_best_stream(format.get(), AVMEDIA_TYPE_AUDIO, -1, videoStream, nullptr, 0);
	AVStream* stream = format->streams[videoStream];
	AVCodecParameters* params = stream->codecpar;
	if (params->codec_id != AV_CODEC_ID_H264) {
		SetError("unsupported video codec; H.264/AVC is required");
		return;
	}
	if (params->width <= 0 || params->height <= 0 || params->width > maxWidth || params->height > maxHeight) {
		SetError("video dimensions exceed safe limits");
		return;
	}
	if (audioStream >= 0 && format->streams[audioStream]->codecpar->codec_id != AV_CODEC_ID_AAC) {
		SetError("unsupported audio codec; AAC-LC or no audio is required");
		return;
	}
	if (audioStream >= 0) {
		const int profile = format->streams[audioStream]->codecpar->profile;
		if (profile != FF_PROFILE_UNKNOWN && profile != FF_PROFILE_AAC_LOW) {
			SetError("unsupported AAC profile; AAC-LC stereo is required");
			return;
		}
	}

	const AVCodec* codec = avcodec_find_decoder(params->codec_id);
	if (codec == nullptr) {
		SetError("FFmpeg H.264 decoder is unavailable");
		return;
	}
	std::unique_ptr<AVCodecContext, CodecDeleter> codecContext(avcodec_alloc_context3(codec));
	if (codecContext == nullptr || avcodec_parameters_to_context(codecContext.get(), params) < 0) {
		SetError("could not create H.264 decoder context");
		return;
	}
	codecContext->thread_count = decoderThreads;
	result = avcodec_open2(codecContext.get(), codec, nullptr);
	if (result < 0) {
		SetError("could not open H.264 decoder: " + FFmpegError(result));
		return;
	}

	std::unique_ptr<AVCodecContext, CodecDeleter> audioCodecContext;
	SwrContext* resampler = nullptr;
	AVStream* audioAVStream = nullptr;
	if (audioStream >= 0 && options.audio) {
		audioAVStream = format->streams[audioStream];
		const AVCodec* audioCodec = avcodec_find_decoder(audioAVStream->codecpar->codec_id);
		audioCodecContext.reset(avcodec_alloc_context3(audioCodec));
		if (audioCodec == nullptr || audioCodecContext == nullptr ||
			avcodec_parameters_to_context(audioCodecContext.get(), audioAVStream->codecpar) < 0 ||
			audioCodecContext->ch_layout.nb_channels != 2 ||
			avcodec_open2(audioCodecContext.get(), audioCodec, nullptr) < 0) {
			SetError("AAC-LC stereo audio stream could not be opened");
			return;
		}

		AVChannelLayout outputLayout = AV_CHANNEL_LAYOUT_STEREO;
		result = swr_alloc_set_opts2(&resampler, &outputLayout, AV_SAMPLE_FMT_S16, 48000,
			&audioCodecContext->ch_layout, audioCodecContext->sample_fmt, audioCodecContext->sample_rate, 0, nullptr);
		av_channel_layout_uninit(&outputLayout);
		if (result < 0 || resampler == nullptr || swr_init(resampler) < 0) {
			swr_free(&resampler);
			SetError("could not initialize AAC audio resampler");
			return;
		}

	} else if (pcmStream != nullptr) {
		pcmStream->Close();
		pcmStream.reset();
	}

	std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
	std::unique_ptr<AVFrame, FrameDeleter> decoded(av_frame_alloc());
	std::unique_ptr<AVFrame, FrameDeleter> decodedAudio(av_frame_alloc());
	if (packet == nullptr || decoded == nullptr || decodedAudio == nullptr) {
		swr_free(&resampler);
		SetError("could not allocate decoder frames");
		return;
	}

	SwsContext* scaler = nullptr;
	const std::int64_t streamStart = (stream->start_time == AV_NOPTS_VALUE) ? 0 : stream->start_time;
	bool autoplay = true;
	float initialVolume = 1.0f;
	{
		std::lock_guard<std::mutex> lock(mutex);
		width = params->width;
		height = params->height;
		hasAudio = (audioStream >= 0);
		if (stream->duration != AV_NOPTS_VALUE)
			durationUs = av_rescale_q(stream->duration, stream->time_base, AVRational{1, 1000000});
		else if (format->duration != AV_NOPTS_VALUE)
			durationUs = format->duration;
		autoplay = options.autoplay;
		initialVolume = options.volume;
		state = autoplay ? PlaybackState::Playing : PlaybackState::Ready;
		StartClockLocked(0);
	}
	if (pcmStream != nullptr) {
		pcmStream->SetVolume(initialVolume);
		if (autoplay)
			pcmStream->Play();
		else
			pcmStream->Pause();
	}

	const std::int64_t timelineStartUs = (format->start_time == AV_NOPTS_VALUE)
		? av_rescale_q(streamStart, stream->time_base, AVRational{1, 1000000})
		: format->start_time;
	std::int64_t audioDiscardBeforeUs = 0;
	std::int64_t videoDiscardBeforeUs = 0;
	std::int64_t nextAudioPtsUs = 0;
	std::int64_t nextVideoPtsUs = 0;
	const AVRational guessedRate = av_guess_frame_rate(format.get(), stream, nullptr);
	const std::int64_t frameDurationUs = (guessedRate.num > 0 && guessedRate.den > 0)
		? av_rescale_q(1, AVRational{guessedRate.den, guessedRate.num}, AVRational{1, 1000000})
		: 33333;
	auto DrainAudio = [&]() -> bool {
		if (audioCodecContext == nullptr)
			return true;
		int audioResult = 0;
		while ((audioResult = avcodec_receive_frame(audioCodecContext.get(), decodedAudio.get())) >= 0) {
			ZoneNamedNC(tracyAudioDecodeFrame, "AudioDecodeFrame", tracy::Color::Orange, true);
			const int outputSamples = av_rescale_rnd(
				swr_get_delay(resampler, audioCodecContext->sample_rate) + decodedAudio->nb_samples,
				48000, audioCodecContext->sample_rate, AV_ROUND_UP);
			PCMBlock block;
			block.sampleRate = 48000;
			block.channels = 2;
			block.samples.resize(static_cast<std::size_t>(outputSamples) * block.channels);
			std::uint8_t* output[] = {reinterpret_cast<std::uint8_t*>(block.samples.data())};
			const std::uint8_t* input[AV_NUM_DATA_POINTERS] = {};
			for (std::size_t i = 0; i < std::size(input); ++i)
				input[i] = decodedAudio->extended_data[i];
			int converted = 0;
			{
				ZoneNamedNC(tracyAudioResample, "AudioResample", tracy::Color::DarkOrange, true);
				converted = swr_convert(resampler, output, outputSamples,
					input, decodedAudio->nb_samples);
			}
			if (converted < 0) {
				SetError("AAC resampling failed: " + FFmpegError(converted));
				return false;
			}
			block.samples.resize(static_cast<std::size_t>(converted) * block.channels);
			if (decodedAudio->best_effort_timestamp == AV_NOPTS_VALUE) {
				block.ptsUs = nextAudioPtsUs;
			} else {
				block.ptsUs = std::max<std::int64_t>(0,
					av_rescale_q(decodedAudio->best_effort_timestamp, audioAVStream->time_base, AVRational{1, 1000000}) - timelineStartUs);
			}
			nextAudioPtsUs = block.ptsUs + converted * 1000000LL / block.sampleRate;
			av_frame_unref(decodedAudio.get());
			if (audioDiscardBeforeUs > block.ptsUs) {
				const std::int64_t samplesToDiscard = std::min<std::int64_t>(
					converted, (audioDiscardBeforeUs - block.ptsUs) * block.sampleRate / 1000000);
				if (samplesToDiscard >= converted)
					continue;
				block.samples.erase(block.samples.begin(), block.samples.begin() + samplesToDiscard * block.channels);
				block.ptsUs += samplesToDiscard * 1000000 / block.sampleRate;
				audioDiscardBeforeUs = 0;
			}

			while (pcmStream != nullptr && !pcmStream->Queue(std::move(block))) {
				if (pcmStream->IsClosed()) {
					LOG_L(L_WARNING, "[VideoPlayer] audio output unavailable for %s; continuing silently", path.c_str());
					std::lock_guard<std::mutex> lock(mutex);
					pcmStream.reset();
					break;
				}
				{
					std::lock_guard<std::mutex> lock(mutex);
					if (shutdown || requestedSeekUs.has_value())
						return true;
				}
				spring::this_thread::sleep_for(std::chrono::milliseconds(2));
			}
		}
		if (audioResult == AVERROR(EAGAIN) || audioResult == AVERROR_EOF)
			return true;
		SetError("AAC frame decode failed: " + FFmpegError(audioResult));
		return false;
	};

	constexpr int QUEUE_FULL = 1;
	auto DrainVideo = [&]() -> int {
		int receiveResult = 0;
		while ((receiveResult = avcodec_receive_frame(codecContext.get(), decoded.get())) >= 0) {
			ZoneNamedNC(tracyVideoDecodeFrame, "VideoDecodeFrame", tracy::Color::MediumAquamarine, true);
			bool interlaced = false;
#ifdef AV_FRAME_FLAG_INTERLACED
			interlaced = (decoded->flags & AV_FRAME_FLAG_INTERLACED) != 0;
#else
			interlaced = decoded->interlaced_frame != 0;
#endif
			if (decoded->width != width || decoded->height != height || decoded->format != AV_PIX_FMT_YUV420P || interlaced) {
				SetError("decoded stream changed dimensions or is not progressive 8-bit yuv420p");
				return AVERROR_INVALIDDATA;
			}
			if (scaler == nullptr) {
				scaler = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_BGRA,
					SWS_BILINEAR, nullptr, nullptr, nullptr);
				if (scaler == nullptr) {
					SetError("could not create video color converter");
					return AVERROR(ENOMEM);
				}
				const int* coefficients = sws_getCoefficients(decoded->colorspace == AVCOL_SPC_BT470BG ? SWS_CS_ITU601 : SWS_CS_ITU709);
				sws_setColorspaceDetails(scaler, coefficients, decoded->color_range == AVCOL_RANGE_JPEG,
					coefficients, 1, 0, 1 << 16, 1 << 16);
			}

			VideoFrame output;
			output.width = width;
			output.height = height;
			output.bgra.resize(static_cast<std::size_t>(width) * height * 4);
			std::uint8_t* destination[] = {output.bgra.data()};
			int destinationStride[] = {width * 4};
			{
				ZoneNamedNC(tracyVideoColorConvert, "VideoColorConvert", tracy::Color::DarkTurquoise, true);
				sws_scale(scaler, decoded->data, decoded->linesize, 0, height, destination, destinationStride);
			}
			if (decoded->best_effort_timestamp == AV_NOPTS_VALUE) {
				output.ptsUs = nextVideoPtsUs;
			} else {
				output.ptsUs = std::max<std::int64_t>(0,
					av_rescale_q(decoded->best_effort_timestamp, stream->time_base, AVRational{1, 1000000}) - timelineStartUs);
			}
			nextVideoPtsUs = output.ptsUs + frameDurationUs;
			av_frame_unref(decoded.get());
			if (videoDiscardBeforeUs > 0 && output.ptsUs < videoDiscardBeforeUs)
				continue;
			videoDiscardBeforeUs = 0;

			std::lock_guard<std::mutex> lock(mutex);
			frames.emplace_back(std::move(output));
			if (frames.size() >= maxQueuedFrames)
				return QUEUE_FULL;
		}
		if (receiveResult != AVERROR(EAGAIN) && receiveResult != AVERROR_EOF)
			SetError("H.264 frame decode failed: " + FFmpegError(receiveResult));
		return receiveResult;
	};

	bool demuxEOF = false;
	bool videoFlushSent = false;
	bool audioFlushSent = (audioCodecContext == nullptr);
	bool videoDecoderEOF = false;
	bool audioDecoderEOF = (audioCodecContext == nullptr);
	bool decodeFinished = false;
	bool packetPending = false;
	while (true) {
		ZoneNamedNC(tracyDecodeLoop, "VideoDecodeLoop", tracy::Color::MediumAquamarine, true);
		std::optional<std::int64_t> seekUs;
		bool queueFull = false;
		{
			ZoneNamedNC(tracyVideoWait, "VideoWait", tracy::Color::DarkGoldenrod, true);
			std::unique_lock<std::mutex> lock(mutex);
			cv.wait_for(lock, std::chrono::milliseconds(10), [&] {
				return shutdown || requestedSeekUs.has_value() || (!decodeFinished && frames.size() < maxQueuedFrames);
			});
			if (shutdown)
				break;
			const std::int64_t positionUs = PositionUsLocked();
			while (frames.size() > 1 && frames[1].ptsUs <= positionUs) {
				frames.pop_front();
				++droppedFrames;
			}
			seekUs = requestedSeekUs;
			requestedSeekUs.reset();
			queueFull = !seekUs.has_value() && frames.size() >= maxQueuedFrames;
			TracyPlot(videoFrameQueuePlot, static_cast<int64_t>(frames.size()));
			TracyPlot(videoDroppedFramesPlot, static_cast<int64_t>(droppedFrames));
			TracyPlot(videoPositionPlot, PositionUsLocked());
		}
		if (queueFull)
			continue;

		if (seekUs.has_value()) {
			ZoneNamedNC(tracyVideoSeek, "VideoSeek", tracy::Color::Gold, true);
			const std::int64_t clampedUs = std::clamp<std::int64_t>(*seekUs, 0, durationUs > 0 ? durationUs : *seekUs);
			const std::int64_t target = av_rescale_q(clampedUs + timelineStartUs, AVRational{1, 1000000}, stream->time_base);
			result = av_seek_frame(format.get(), videoStream, target, AVSEEK_FLAG_BACKWARD);
			if (result < 0) {
				SetError("seek failed: " + FFmpegError(result));
				break;
			}
			avcodec_flush_buffers(codecContext.get());
			if (audioCodecContext != nullptr) {
				avcodec_flush_buffers(audioCodecContext.get());
				swr_close(resampler);
				if (swr_init(resampler) < 0) {
					SetError("could not reset AAC resampler after seek");
					break;
				}
			}
			if (pcmStream != nullptr)
				pcmStream->Flush(clampedUs);
			videoDiscardBeforeUs = clampedUs;
			audioDiscardBeforeUs = clampedUs;
			nextVideoPtsUs = clampedUs;
			nextAudioPtsUs = clampedUs;
			demuxEOF = false;
			videoFlushSent = false;
			audioFlushSent = (audioCodecContext == nullptr);
			videoDecoderEOF = false;
			audioDecoderEOF = (audioCodecContext == nullptr);
			decodeFinished = false;
			if (packetPending) {
				av_packet_unref(packet.get());
				packetPending = false;
			}
			std::lock_guard<std::mutex> lock(mutex);
			frames.clear();
			StartClockLocked(clampedUs);
			state = seekTargetState;
			if (pcmStream != nullptr) {
				if (state == PlaybackState::Playing)
					pcmStream->Play();
				else
					pcmStream->Pause();
			}
		}

		if (decodeFinished) {
			std::lock_guard<std::mutex> lock(mutex);
			if (PositionUsLocked() >= durationUs) {
				if (options.loop) {
					requestedSeekUs = 0;
					seekTargetState = PlaybackState::Playing;
				} else {
					state = PlaybackState::Finished;
					clockBaseUs = durationUs;
				}
				cv.notify_all();
			}
			continue;
		}

		result = DrainVideo();
		if (result == QUEUE_FULL)
			continue;
		if (result == AVERROR_EOF)
			videoDecoderEOF = true;
		else if (result != AVERROR(EAGAIN))
			break;
		if (!DrainAudio())
			break;
		if (audioCodecContext != nullptr) {
			result = avcodec_receive_frame(audioCodecContext.get(), decodedAudio.get());
			if (result == AVERROR_EOF)
				audioDecoderEOF = true;
			else if (result >= 0)
				av_frame_unref(decodedAudio.get());
			else if (result != AVERROR(EAGAIN)) {
				SetError("AAC frame decode failed: " + FFmpegError(result));
				break;
			}
		}

		if (!packetPending && !demuxEOF) {
			ZoneNamedNC(tracyVideoDemux, "VideoDemux", tracy::Color::MediumPurple, true);
			result = av_read_frame(format.get(), packet.get());
			if (result == AVERROR_EOF) {
				demuxEOF = true;
			} else if (result < 0) {
				SetError("media read failed: " + FFmpegError(result));
				break;
			} else {
				packetPending = true;
			}
		}

		if (packetPending) {
			AVCodecContext* destination = nullptr;
			const char* codecName = nullptr;
			if (packet->stream_index == videoStream) {
				destination = codecContext.get();
				codecName = "H.264";
			} else if (audioCodecContext != nullptr && packet->stream_index == audioStream) {
				destination = audioCodecContext.get();
				codecName = "AAC";
			}
			if (destination == nullptr) {
				av_packet_unref(packet.get());
				packetPending = false;
			} else {
				result = avcodec_send_packet(destination, packet.get());
				if (result == 0) {
					av_packet_unref(packet.get());
					packetPending = false;
				} else if (result != AVERROR(EAGAIN)) {
					SetError(std::string(codecName) + " packet decode failed: " + FFmpegError(result));
					break;
				}
			}
		}

		if (demuxEOF && !packetPending) {
			if (!videoFlushSent) {
				result = avcodec_send_packet(codecContext.get(), nullptr);
				if (result == 0 || result == AVERROR_EOF)
					videoFlushSent = true;
				else if (result != AVERROR(EAGAIN)) {
					SetError("could not flush H.264 decoder: " + FFmpegError(result));
					break;
				}
			}
			if (!audioFlushSent) {
				result = avcodec_send_packet(audioCodecContext.get(), nullptr);
				if (result == 0 || result == AVERROR_EOF)
					audioFlushSent = true;
				else if (result != AVERROR(EAGAIN)) {
					SetError("could not flush AAC decoder: " + FFmpegError(result));
					break;
				}
			}
		}

		if (videoFlushSent && audioFlushSent && videoDecoderEOF && audioDecoderEOF) {
			bool resamplerInterrupted = false;
			while (resampler != nullptr && swr_get_delay(resampler, audioCodecContext->sample_rate) > 0) {
				const int outputSamples = av_rescale_rnd(
					swr_get_delay(resampler, audioCodecContext->sample_rate),
					48000, audioCodecContext->sample_rate, AV_ROUND_UP);
				PCMBlock block;
				block.sampleRate = 48000;
				block.channels = 2;
				block.ptsUs = nextAudioPtsUs;
				block.samples.resize(static_cast<std::size_t>(outputSamples) * block.channels);
				std::uint8_t* output[] = {reinterpret_cast<std::uint8_t*>(block.samples.data())};
				const int converted = swr_convert(resampler, output, outputSamples, nullptr, 0);
				if (converted < 0) {
					SetError("AAC resampler drain failed: " + FFmpegError(converted));
					break;
				}
				if (converted == 0)
					break;
				block.samples.resize(static_cast<std::size_t>(converted) * block.channels);
				nextAudioPtsUs += converted * 1000000LL / block.sampleRate;
				while (pcmStream != nullptr && !pcmStream->Queue(std::move(block))) {
					if (pcmStream->IsClosed())
						break;
					{
						std::lock_guard<std::mutex> lock(mutex);
						if (shutdown || requestedSeekUs.has_value()) {
							resamplerInterrupted = true;
							break;
						}
					}
					spring::this_thread::sleep_for(std::chrono::milliseconds(2));
				}
				if (resamplerInterrupted)
					break;
			}
			if (resamplerInterrupted)
				continue;
			{
				std::lock_guard<std::mutex> lock(mutex);
				if (state == PlaybackState::Error)
					break;
			}
			if (pcmStream != nullptr) {
				PCMBlock end;
				end.endOfStream = true;
				while (!pcmStream->Queue(std::move(end)) && !pcmStream->IsClosed())
					spring::this_thread::sleep_for(std::chrono::milliseconds(2));
			}
			{
				std::lock_guard<std::mutex> lock(mutex);
				durationUs = std::max({durationUs, nextVideoPtsUs, nextAudioPtsUs});
			}
			decodeFinished = true;
		}
	}

	if (scaler != nullptr)
		sws_freeContext(scaler);
	swr_free(&resampler);
	format.reset();
}

#endif

VideoPlayer::VideoPlayer(const std::string& path, const std::string& vfsModes, const VideoOptions& options)
	: impl(std::make_unique<Impl>(path, vfsModes, options))
{}

VideoPlayer::~VideoPlayer() = default;

void VideoPlayer::Play()
{
	std::lock_guard<std::mutex> lock(impl->mutex);
	if (impl->state == PlaybackState::Opening) {
		impl->options.autoplay = true;
		return;
	}
	if (impl->state == PlaybackState::Error)
		return;
	if (impl->state == PlaybackState::Seeking) {
		impl->seekTargetState = PlaybackState::Playing;
		if (impl->pcmStream != nullptr)
			impl->pcmStream->Play();
		return;
	}
	if (impl->state == PlaybackState::Finished || impl->state == PlaybackState::Stopped) {
		impl->requestedSeekUs = 0;
		impl->seekTargetState = PlaybackState::Playing;
		impl->state = PlaybackState::Seeking;
	} else if (impl->state != PlaybackState::Playing) {
		impl->StartClockLocked(impl->clockBaseUs);
		impl->state = PlaybackState::Playing;
	}
	if (impl->pcmStream != nullptr)
		impl->pcmStream->Play();
	impl->cv.notify_all();
}

void VideoPlayer::Pause()
{
	std::lock_guard<std::mutex> lock(impl->mutex);
	if (impl->state == PlaybackState::Opening) {
		impl->options.autoplay = false;
		return;
	}
	if (impl->state == PlaybackState::Seeking) {
		impl->seekTargetState = PlaybackState::Paused;
		if (impl->pcmStream != nullptr)
			impl->pcmStream->Pause();
		return;
	}
	if (impl->state == PlaybackState::Playing) {
		impl->clockBaseUs = impl->PositionUsLocked();
		impl->state = PlaybackState::Paused;
		if (impl->pcmStream != nullptr)
			impl->pcmStream->Pause();
	}
}

void VideoPlayer::Stop()
{
	std::lock_guard<std::mutex> lock(impl->mutex);
	if (impl->state == PlaybackState::Error)
		return;
	impl->frames.clear();
	impl->requestedSeekUs = 0;
	impl->seekTargetState = PlaybackState::Stopped;
	impl->clockBaseUs = 0;
	impl->state = PlaybackState::Stopped;
	if (impl->pcmStream != nullptr) {
		impl->pcmStream->Pause();
		impl->pcmStream->Flush(0);
	}
	impl->cv.notify_all();
}

void VideoPlayer::Seek(double seconds)
{
	if (!std::isfinite(seconds))
		return;
	std::lock_guard<std::mutex> lock(impl->mutex);
	if (impl->state == PlaybackState::Error || impl->state == PlaybackState::Opening)
		return;
	constexpr double maxSeconds = 315576000.0; // Ten years, well beyond practical media duration.
	seconds = std::clamp<double>(seconds, 0.0, maxSeconds);
	if (impl->durationUs > 0)
		seconds = std::min<double>(seconds, impl->durationUs / 1000000.0);
	impl->seekTargetState = (impl->state == PlaybackState::Playing) ? PlaybackState::Playing : PlaybackState::Paused;
	impl->requestedSeekUs = static_cast<std::int64_t>(seconds * 1000000.0);
	impl->state = PlaybackState::Seeking;
	impl->cv.notify_all();
}

void VideoPlayer::SetVolume(float volume)
{
	if (!std::isfinite(volume))
		return;
	std::lock_guard<std::mutex> lock(impl->mutex);
	impl->options.volume = std::clamp(volume, 0.0f, 1.0f);
	if (impl->pcmStream != nullptr)
		impl->pcmStream->SetVolume(impl->options.volume);
}

VideoInfo VideoPlayer::GetInfo() const
{
	std::lock_guard<std::mutex> lock(impl->mutex);
	return {
		.state = impl->state,
		.width = impl->width,
		.height = impl->height,
		.duration = impl->durationUs / 1000000.0,
		.position = impl->PositionUsLocked() / 1000000.0,
		.hasAudio = impl->hasAudio,
		.error = impl->error,
		.droppedFrames = impl->droppedFrames,
	};
}

bool VideoPlayer::TakeDueFrame(VideoFrame& frame)
{
	std::lock_guard<std::mutex> lock(impl->mutex);
	const std::int64_t positionUs = impl->PositionUsLocked();
	bool found = false;
	while (!impl->frames.empty() && impl->frames.front().ptsUs <= positionUs) {
		if (found)
			++impl->droppedFrames;
		frame = std::move(impl->frames.front());
		impl->frames.pop_front();
		found = true;
	}
	if (found)
		impl->cv.notify_all();
	return found;
}

} // namespace video
