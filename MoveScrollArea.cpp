#include "MoveScrollArea.h"
#include <QScroller>

MoveScrollArea::MoveScrollArea(QWidget* parent) :
  QScrollArea(parent)
{
  this->setWidgetResizable(true);
  this->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  this->setFrameShape(QFrame::NoFrame);
  this->setWidget(new QWidget);
  this->widget()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  QScroller::grabGesture(this->viewport(), QScroller::LeftMouseButtonGesture);
}

void MoveScrollArea::setLayout(QLayout* l) {
  widget()->setLayout(l);
}
