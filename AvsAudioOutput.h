#ifndef AVSAUDIOOUTPUT_H
#define AVSAUDIOOUTPUT_H

#include "avisynth.h"
#include <QByteArray>
#include <QObject>
#include <QTimer>
#include <QVector>

class QAudioSink;
class QIODevice;

// plays the clip's audio through a QAudioSink: 16 bit, at most two channels (more are mixed down), volume 100 %
class AvsAudioOutput : public QObject
{
  Q_OBJECT
  public:
    explicit AvsAudioOutput(QObject* parent = nullptr);
    ~AvsAudioOutput();
    // from firstSample on; false (with the reason in error) when there is nothing to play or the device rejects the format
    bool start(const PClip& clip, IScriptEnvironment* env, const VideoInfo& vi, int64_t firstSample, QString& error);
    void stop();
    bool isActive() const;
    // seconds played since start(): what left the sink's buffer
    double clock() const;
    // every sample was written and the sink played its buffer empty
    bool finished() const;

  signals:
    // after each feed tick (every 10 ms), the caller checks whether the next frame is due
    void fed();

  private slots:
    void tick();

  private:
    // fills the sink without signalling: start() calls it too, and a receiver of fed() may call start()
    void feed();

  private:
    PClip m_clip;
    IScriptEnvironment* m_env = nullptr;
    QAudioSink* m_sink = nullptr;
    QIODevice* m_device = nullptr;
    QTimer m_timer;
    int64_t m_first = 0;
    int64_t m_next = 0;
    int64_t m_total = 0;
    int m_rate = 0;
    int m_inChannels = 0;
    int m_outChannels = 0;
    int m_sampleType = 0;
    int m_bytesPerSample = 0;
    QVector<float> m_matrix; // weight of input channel i in output channel o at [o * m_inChannels + i]
    QByteArray m_input;
    QByteArray m_pending;
    void setMatrix(unsigned int channelMask, bool maskKnown);
    float sample(const char* at) const;
    QByteArray convert(int64_t count) const;
};

#endif // AVSAUDIOOUTPUT_H
