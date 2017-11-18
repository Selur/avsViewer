/*
 * MarkSlider.cpp
 *
 *  Created on: Feb 20, 2012
 *      Author: Selur
 */

#include "MarkSlider.h"
#include <QPaintEvent>
#include <QPainter>

MarkSlider::MarkSlider(QWidget *parent)
    : QSlider(parent), m_start(0), m_end(0), m_sections()
{
}

MarkSlider::~MarkSlider()
{
}

void MarkSlider::paintEvent(QPaintEvent *event)
{
  QSlider::paintEvent(event);
  if (m_start == 0 && m_end == 0) {
    return;
  }
  if (m_start != this->minimum() || m_end != this->maximum()) {
    int left = (m_start * width()) / this->maximum();
    int right = (m_end * width()) / this->maximum();
    QPainter painter(this);
    QColor color = Qt::darkBlue;
    color.setAlpha(50);
    painter.fillRect(left, 1, right - left, height() - 3, color);
    painter.end();
  }
  if (m_sections.isEmpty()) {
    return;
  }
  QStringList elements;
  QPainter painter(this);
  QColor color = Qt::darkGray;
  color.setAlpha(50);
  int left, right = (m_end * width()) / this->maximum();
  foreach(QString section, m_sections)
  {
    elements = section.split("-");
    left = (elements.at(0).toInt() * width()) / this->maximum();
    right = (elements.at(1).toInt() * width()) / this->maximum();
    painter.fillRect(left, 1, right - left, height() - 3, color);
  }
  painter.end();
}

void MarkSlider::multiMark(QStringList sections)
{
  if (sections.isEmpty()) {
    return;
  }
  m_sections = sections;
  this->repaint();
}

void MarkSlider::resetMarks()
{
  m_start = this->minimum();
  m_end = this->maximum();
}

int MarkSlider::getEnd() const
{
  return m_end;
}

int MarkSlider::getStart() const
{
  return m_start;
}

void MarkSlider::setEnd(int end)
{
  if (end > this->maximum() || end < this->minimum()) {
    return;
  }
  if (end == m_start && end == m_end) {
    return;
  }
  if (end < m_start) {
    m_end = m_start;
    m_start = end;
  } else {
    m_end = end;
  }
  this->repaint();
}

void MarkSlider::setStart(int start)
{
  if (start > this->maximum() || start < this->minimum()) {
    return;
  }
  if (start == m_start && start == m_end) {
    return;
  }
  if (start > m_end) {
    m_start = m_end;
    m_end = start;
  } else {
    m_start = start;
  }
  this->repaint();
}

