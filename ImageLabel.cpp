/*
 * ImageLabel.cpp
 *
 *  Created on: Feb 19, 2012
 *      Author: Selur
 */

#include "ImageLabel.h"
#include "ImageResizer.h"
#include <QPainter>

ImageLabel::ImageLabel(QWidget * parent, Qt::WindowFlags f)
    : QLabel(parent, f), m_source(), m_current(), m_interpolation("none"), m_resizer(NULL)
{
  m_resizer = new ImageResizer(this);
}

ImageLabel::~ImageLabel()
{

}

void ImageLabel::paintEvent(QPaintEvent *aEvent)
{
  QLabel::paintEvent(aEvent);
  this->displayImage();
}

void ImageLabel::setPixmap(QPixmap aPicture)
{
  m_source = m_current = aPicture;
  repaint();
}

void ImageLabel::setInterpolation(QString interpolation)
{
  m_interpolation = interpolation;
}

void ImageLabel::displayImage()
{
  if (m_source.isNull()) { //no image was set, don't draw anything
    return;
  }
  float cw = width(), ch = height();
  float pw = m_current.width(), ph = m_current.height();
  if (m_interpolation == "bicubic") {
    QImage image = m_source.toImage();
    m_resizer->setImage(&image);
    if ((pw > cw && ph > ch && pw / cw > ph / ch) || //both width and high are bigger, ratio at high is bigger or
        (pw > cw && ph <= ch) || //only the width is bigger or
        (pw < cw && ph < ch && cw / pw < ch / ph) //both width and height is smaller, ratio at width is smaller
        ) {
      // width defines the mult
      image = m_resizer->resizeTo(cw, ph * cw / pw, "bicubic");
    } else if ((pw > cw && ph > ch && pw / cw <= ph / ch) || //both width and high are bigger, ratio at width is bigger or
        (ph > ch && pw <= cw) || //only the height is bigger or
        (pw < cw && ph < ch && cw / pw > ch / ph) //both width and height is smaller, ratio at height is smaller
        ) {
      // hight defines the mult
      image = m_resizer->resizeTo(pw * ch / ph, ch, "bicubic");
    }
    m_current = QPixmap::fromImage(image);
  } else {
    if ((pw > cw && ph > ch && pw / cw > ph / ch) || //both width and high are bigger, ratio at high is bigger or
        (pw > cw && ph <= ch) || //only the width is bigger or
        (pw < cw && ph < ch && cw / pw < ch / ph) //both width and height is smaller, ratio at width is smaller
        ) {
      if (m_interpolation == "bilinear") {
        m_current = m_source.scaledToWidth(cw, Qt::SmoothTransformation);
      } else {
        m_current = m_source.scaledToWidth(cw, Qt::FastTransformation);
      }
    } else if ((pw > cw && ph > ch && pw / cw <= ph / ch) || //both width and high are bigger, ratio at width is bigger or
        (ph > ch && pw <= cw) || //only the height is bigger or
        (pw < cw && ph < ch && cw / pw > ch / ph) //both width and height is smaller, ratio at height is smaller
        ) {
      if (m_interpolation == "bilinear") {
        m_current = m_source.scaledToHeight(ch, Qt::SmoothTransformation);
      } else {
        m_current = m_source.scaledToHeight(ch, Qt::FastTransformation);
      }
    }
  }
  int x = (cw - m_current.width()) / 2, y = (ch - m_current.height()) / 2;
  QPainter paint(this);
  paint.setRenderHints(
      QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing);
  paint.drawPixmap(x, y, m_current);
}
