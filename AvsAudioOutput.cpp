#include "AvsAudioOutput.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QMediaDevices>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

static const float HALF_POWER = 0.70710678f;

AvsAudioOutput::AvsAudioOutput(QObject* parent) : QObject(parent)
{
  m_timer.setInterval(10);
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &AvsAudioOutput::tick);
}

AvsAudioOutput::~AvsAudioOutput()
{
  this->stop();
}

bool AvsAudioOutput::start(const PClip& clip, IScriptEnvironment* env, const VideoInfo& vi, int64_t firstSample, QString& error)
{
  this->stop();
  if (!vi.HasAudio() || vi.AudioChannels() < 1) {
    error = tr("the clip has no audio");
    return false;
  }
  if (firstSample >= vi.num_audio_samples) {
    error = tr("the audio ends before this frame");
    return false;
  }
  m_rate = vi.audio_samples_per_second;
  m_inChannels = vi.AudioChannels();
  m_outChannels = std::min(2, m_inChannels);
  m_sampleType = vi.SampleType();
  m_bytesPerSample = vi.BytesPerChannelSample();
  this->setMatrix(vi.GetChannelMask(), vi.IsChannelMaskKnown());

  QAudioFormat format;
  format.setSampleRate(m_rate);
  format.setChannelCount(m_outChannels);
  format.setSampleFormat(QAudioFormat::Int16);
  const QAudioDevice device = QMediaDevices::defaultAudioOutput();
  if (device.isNull()) {
    error = tr("no audio output device");
    return false;
  }
  if (!device.isFormatSupported(format)) {
    error = tr("the audio device does not take %1 Hz, %2 channel(s), 16 bit").arg(m_rate).arg(m_outChannels);
    return false;
  }
  m_sink = new QAudioSink(device, format, this);
  m_sink->setBufferSize(m_rate * m_outChannels * 2); // 1 s: the GUI thread decodes the picture, a 4K/5K frame can hold it for half a second
  m_device = m_sink->start();
  if (m_device == nullptr) {
    error = tr("the audio device could not be opened");
    this->stop();
    return false;
  }
  m_clip = clip;
  m_env = env;
  m_first = firstSample;
  m_next = firstSample;
  m_total = vi.num_audio_samples;
  m_timer.start();
  this->feed();
  return true;
}

void AvsAudioOutput::stop()
{
  m_timer.stop();
  if (m_sink != nullptr) {
    // reset(), not stop(): stop() drains, the queued second keeps playing after the sink is gone and overlaps the next start (Qt 6.10.1)
    m_sink->reset();
    delete m_sink;
    m_sink = nullptr;
    m_device = nullptr;
  }
  m_pending.clear();
  m_clip = nullptr; // the clip has to go before its script environment does
  m_env = nullptr;
}

bool AvsAudioOutput::isActive() const
{
  return m_sink != nullptr;
}

double AvsAudioOutput::clock() const
{
  if (m_sink == nullptr) {
    return 0.0;
  }
  // measured (Windows backend, Qt 6.10.1): processedUSecs() already leaves out what waits in the sink's buffer
  return double(m_sink->processedUSecs()) / 1e6;
}

bool AvsAudioOutput::finished() const
{
  return m_sink != nullptr && m_next >= m_total && m_pending.isEmpty() && m_sink->bytesFree() >= m_sink->bufferSize();
}

// as much as the sink takes, read in chunks of at most 50 ms
void AvsAudioOutput::feed()
{
  if (m_sink == nullptr || m_device == nullptr) {
    return;
  }
  try {
    while (true) {
      if (!m_pending.isEmpty()) {
        const qint64 written = m_device->write(m_pending.constData(), std::min<qint64>(m_sink->bytesFree(), m_pending.size()));
        if (written > 0) {
          m_pending.remove(0, int(written));
        }
        if (!m_pending.isEmpty()) {
          break;
        }
      }
      const int64_t room = m_sink->bytesFree() / (m_outChannels * 2);
      if (room <= 0 || m_next >= m_total) {
        break;
      }
      // the Windows backend keeps reporting room while it hands data on, so the read-ahead is bounded by what has played
      if (double(m_next - m_first) / m_rate - this->clock() >= 1.0) {
        break;
      }
      const int64_t count = std::min<int64_t>({room, int64_t(m_rate / 20), m_total - m_next});
      m_input.resize(int(count * m_inChannels * m_bytesPerSample));
      m_clip->GetAudio(m_input.data(), m_next, count, m_env);
      m_pending = this->convert(count);
      m_next += count;
    }
  } catch (AvisynthError err) {
    std::fprintf(stderr, "audio: %s\n", err.msg);
    m_next = m_total;
    m_pending.clear();
  }
}

void AvsAudioOutput::tick()
{
  this->feed();
  emit fed(); // last: a receiver may stop() or start() this output
}

// stereo and mono pass through; more channels fold to stereo by position, LFE dropped, level halved against clipping
void AvsAudioOutput::setMatrix(unsigned int channelMask, bool maskKnown)
{
  m_matrix.fill(0.0f, m_outChannels * m_inChannels);
  if (m_inChannels <= 2) {
    for (int c = 0; c < m_inChannels; ++c) {
      m_matrix[c * m_inChannels + c] = 1.0f;
    }
    return;
  }
  static const unsigned int defaults[] = {0, 0x4, 0x3, 0x7, 0x33, 0x37, 0x3F, 0x70F, 0x63F};
  unsigned int mask = maskKnown ? channelMask : (m_inChannels <= 8 ? defaults[m_inChannels] : 0);
  for (int c = 0; c < m_inChannels; ++c) {
    int bit = -1;
    if (mask != 0) {
      bit = 0;
      while (((mask >> bit) & 1u) == 0) {
        ++bit;
      }
      mask &= ~(1u << bit);
    }
    float left = HALF_POWER, right = HALF_POWER; // unknown positions go to both sides
    switch (bit) {
      case 0: left = 1.0f; right = 0.0f; break;                        // front left
      case 1: left = 0.0f; right = 1.0f; break;                        // front right
      case 3: left = 0.0f; right = 0.0f; break;                        // LFE
      case 4: case 6: case 9: left = HALF_POWER; right = 0.0f; break;  // back, front-centre and side left
      case 5: case 7: case 10: left = 0.0f; right = HALF_POWER; break; // back, front-centre and side right
      default: break;                                                  // front and back centre, top
    }
    m_matrix[0 * m_inChannels + c] = 0.5f * left;
    m_matrix[1 * m_inChannels + c] = 0.5f * right;
  }
}

float AvsAudioOutput::sample(const char* at) const
{
  switch (m_sampleType) {
    case SAMPLE_INT8: return (float(*reinterpret_cast<const uint8_t*>(at)) - 128.0f) / 128.0f;
    case SAMPLE_INT16: return float(*reinterpret_cast<const int16_t*>(at)) / 32768.0f;
    case SAMPLE_INT24: {
      const uint8_t* b = reinterpret_cast<const uint8_t*>(at);
      const int32_t value = int32_t(uint32_t(b[0]) << 8 | uint32_t(b[1]) << 16 | uint32_t(b[2]) << 24) >> 8;
      return float(value) / 8388608.0f;
    }
    case SAMPLE_INT32: return float(double(*reinterpret_cast<const int32_t*>(at)) / 2147483648.0);
    default: {
      float value;
      std::memcpy(&value, at, sizeof(value));
      return value;
    }
  }
}

// interleaved input of any sample type to interleaved 16 bit output through the channel matrix
QByteArray AvsAudioOutput::convert(int64_t count) const
{
  QByteArray out(int(count * m_outChannels * 2), Qt::Uninitialized);
  int16_t* target = reinterpret_cast<int16_t*>(out.data());
  const char* source = m_input.constData();
  const int frameBytes = m_inChannels * m_bytesPerSample;
  float in[32];
  for (int64_t s = 0; s < count; ++s) {
    const char* frame = source + s * frameBytes;
    const int used = std::min(m_inChannels, 32);
    for (int c = 0; c < used; ++c) {
      in[c] = this->sample(frame + c * m_bytesPerSample);
    }
    for (int o = 0; o < m_outChannels; ++o) {
      float mixed = 0.0f;
      for (int c = 0; c < used; ++c) {
        mixed += m_matrix[o * m_inChannels + c] * in[c];
      }
      mixed = std::clamp(mixed, -1.0f, 1.0f);
      target[s * m_outChannels + o] = int16_t(std::lround(mixed * 32767.0f));
    }
  }
  return out;
}
