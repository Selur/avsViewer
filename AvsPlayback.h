#ifndef AVSPLAYBACK_H
#define AVSPLAYBACK_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

// paces playback at the clip rate: each frame is due one period after the previous one was due
class AvsPlayback : public QObject
{
  Q_OBJECT
  public:
    explicit AvsPlayback(QObject* parent = nullptr);
    void start(unsigned int fpsNumerator, unsigned int fpsDenominator);
    void stop();
    bool isPlaying() const;

  signals:
    // show the next frame now; the receiver decodes it before returning
    void advance();
    void stopped(const QString& stats);

  private slots:
    void tick();

  private:
    QTimer m_timer;
    QElapsedTimer m_clock;
    qint64 m_periodNs = 0;
    qint64 m_dueNs = 0;
    qint64 m_shown = 0;
    qint64 m_late = 0;
    double m_fps = 0.0;
    void scheduleNext();
};

#endif // AVSPLAYBACK_H
