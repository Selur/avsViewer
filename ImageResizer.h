/*
 * ImageResizer.h
 *
 *  Created on: 03.03.2013
 *      Author: Selur
 */

#ifndef IMAGERESIZER_H_
#define IMAGERESIZER_H_

#include <QObject>
#include <QVector>
#include <QImage>
#include <QRgb>

class ImageResizer : public QObject
{
    Q_OBJECT
  public:
    ImageResizer(QObject *parent = 0);
    virtual ~ImageResizer();
    void setImage(QImage *image);
    QImage resizeTo(int newWidth, int newHeight, QString method);

  private:
    QImage *m_image;
    QVector<double> createPolynomial(float x);
    double calculatePolynomial(float x, const QVector<double>& polynomial);
    double BiCubicKernel(double x);
    QRgb BiCubic(double i, double j);
};

#endif /* IMAGERESIZER_H_ */
