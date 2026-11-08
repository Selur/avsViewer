#include "AvsPlayback.h"

AvsPlayback::AvsPlayback(QObject* parent) : QObject(parent)
{
  m_timer.setSingleShot(true);
  m_timer.setTimerType(Qt::PreciseTimer);
  connect(&m_timer, &QTimer::timeout, this, &AvsPlayback::tick);
}

void AvsPlayback::start(unsigned int fpsNumerator, unsigned int fpsDenominator)
{
  if (fpsNumerator == 0 || fpsDenominator == 0) {
    return;
  }
  m_fps = double(fpsNumerator) / double(fpsDenominator);
  m_periodNs = qint64(1000000000.0 * double(fpsDenominator) / double(fpsNumerator) + 0.5);
  m_shown = 0;
  m_late = 0;
  m_clock.start();
  m_dueNs = 0;
  m_timer.start(0);
}

void AvsPlayback::stop()
{
  if (!m_clock.isValid()) {
    return;
  }
  m_timer.stop();
  const double seconds = double(m_clock.nsecsElapsed()) / 1e9;
  m_clock.invalidate();
  const QString stats = QString("playback: %1 frames in %2 s = %3 fps (clip %4 fps), %5 shown more than one period late")
                          .arg(m_shown).arg(seconds, 0, 'f', 2).arg(seconds > 0.0 ? double(m_shown) / seconds : 0.0, 0, 'f', 2)
                          .arg(m_fps, 0, 'f', 3).arg(m_late);
  emit stopped(stats);
}

bool AvsPlayback::isPlaying() const
{
  return m_clock.isValid();
}

void AvsPlayback::tick()
{
  if (!m_clock.isValid()) {
    return;
  }
  if (m_clock.nsecsElapsed() - m_dueNs > m_periodNs) {
    ++m_late;
  }
  emit advance();
  if (!m_clock.isValid()) { // the receiver stopped playback
    return;
  }
  ++m_shown;
  this->scheduleNext();
}

// more than one period behind: the schedule restarts from now instead of rushing to catch up
void AvsPlayback::scheduleNext()
{
  m_dueNs += m_periodNs;
  const qint64 now = m_clock.nsecsElapsed();
  if (now - m_dueNs > m_periodNs) {
    m_dueNs = now;
  }
  const qint64 waitMs = (m_dueNs - now) / 1000000;
  m_timer.start(int(qMax<qint64>(0, waitMs)));
}
