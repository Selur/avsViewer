#ifndef MOVESCROLLAREA_H
#define MOVESCROLLAREA_H

#include <QScrollArea>

class MoveScrollArea : public QScrollArea
{
  public:
    MoveScrollArea(QWidget* parent = 0);

  public slots:
    void setLayout(QLayout* l);
};

#endif // MOVESCROLLAREA_H
