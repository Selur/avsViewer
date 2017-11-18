/*
 * ImageResizer.cpp
 *
 *  Created on: 03.03.2013
 *      Author: Selur
 */

#include "ImageResizer.h"
#include <QImage>

ImageResizer::ImageResizer(QObject *parent)
    : QObject(parent), m_image(NULL)
{
}

ImageResizer::~ImageResizer()
{
}

void ImageResizer::setImage(QImage *image)
{
  m_image = image;
}
void ForwardStroke(QVector<QVector<double> >& a, QVector<double>& b)
{
  int n = a.size();
  int im, i, j;
  double v;
  for (int k = 0; k < n - 1; ++k) {
    im = k;
    for (i = k + 1; i < n; i++) {
      if (qAbs(a[im][k]) < qAbs(a[i][k]))
        im = i;
    }
    if (im != k) {
      for (j = 0; j < n; ++j) {
        qSwap(a[im][j], a[k][j]);
      }
      qSwap(b[im], b[k]);
    }

    for (i = k + 1; i < n; ++i) {
      v = 1.0 * a[i][k] / a[k][k];
      a[i][k] = 0;
      b[i] = b[i] - v * b[k];
      if (v == 0) {
        continue;
      }
      for (j = k + 1; j < n; ++j) {
        a[i][j] = a[i][j] - v * a[k][j];
      }
    }
  }
}

void BackwardStroke(QVector<QVector<double> >& a, QVector<double>& b, QVector<double> &x)
{
  int n = a.size();
  x[n - 1] = 1.0 * b[n - 1] / a[n - 1][n - 1];
  double s;
  for (int i = n - 2, j; i >= 0; --i) {
    s = 0;
    for (j = i + 1; j < n; ++j) {
      s += a[i][j] * x[j];
    }
    x[i] = 1.0 * (b[i] - s) / a[i][i];
  }
}

QVector<double> gauss(QVector<QVector<double> >& matrix, QVector<double>& value)
{

  QVector<double> x;
  x.resize(matrix.size());

  ForwardStroke(matrix, value);
  BackwardStroke(matrix, value, x);

  return x;
}

QRgb ImageResizer::BiCubic(double i, double j)
{

  QVector<QVector<double> > horizontalMatrix;
  QVector<double> horizontalB;

  for (int y = 0; y < 4; ++y) {

    QVector<QVector<double> > matrix;
    QVector<double> b;

    for (int x = 0; x < 4; ++x) {

      matrix.push_back(createPolynomial(x + i));
      b.push_front(y + j);
    }

    QVector<double> solution = gauss(matrix, b);

    horizontalMatrix.push_front(createPolynomial(calculatePolynomial(i, solution)));
    horizontalB.push_front(i);
  }

  QVector<double> verticalPolyomial = gauss(horizontalMatrix, horizontalB);

  double y = calculatePolynomial(i, verticalPolyomial);

  return m_image->pixel(i, y);
}

double ImageResizer::BiCubicKernel(double x)
{
  if (x > 2.0) {
    return 0.0;
  }
  double a, b, c, d;

  double xm1 = x - 1.0, xp1 = x + 1.0, xp2 = x + 2.0;

  a = (xp2 <= 0.0) ? 0.0 : xp2 * xp2 * xp2;
  b = (xp1 <= 0.0) ? 0.0 : xp1 * xp1 * xp1;
  c = (x <= 0.0) ? 0.0 : x * x * x;
  d = (xm1 <= 0.0) ? 0.0 : xm1 * xm1 * xm1;

  return (0.16666666666666666667 * (a - (4.0 * b) + (6.0 * c) - (4.0 * d)));
}

double ImageResizer::calculatePolynomial(float x, const QVector<double>& polynomial)
{
  if (polynomial.size() != 4) {
    return 0;
  }

  double y = polynomial[3], exp = 1;

  for (int i = 2; i >= 0; --i) {
    exp *= x;
    y += polynomial[i] * exp;
  }

  return y;
}

QVector<double> ImageResizer::createPolynomial(float x)
{
  QVector<double> polynomial;

  double koeff = 1;
  for (int i = 0; i < 4; ++i) {
    polynomial.push_front(koeff);
    koeff *= x;
  }

  return polynomial;
}

QImage ImageResizer::resizeTo(int newWidth, int newHeight, QString method)
{
  if (m_image == NULL) {
    return QImage(newWidth, newHeight, QImage::Format_ARGB32);
  }
  QImage image(newWidth, newHeight, m_image->format());
  if (method != "bicubic") {
    return image;
  }
  double xFactor = (double) m_image->width() / newWidth;
  double yFactor = (double) m_image->height() / newHeight;

  int yMax = m_image->height() - 1;
  int xMax = m_image->width() - 1;

  double oy, dy, ox, dx, red, green, blue, k1, oy2, k2, ox2;
  int x, oy1, ox1, m;

  for (int y = 0; y < newHeight; ++y) {

    oy = (double) y * yFactor - 0.5;
    oy1 = (int) oy;
    dy = oy - (double) oy1;

    for (x = 0; x < newWidth; ++x) {

      ox = (double) x * xFactor - 0.5;
      ox1 = (int) ox;
      dx = ox - (double) ox1;
      red = 0, green = 0, blue = 0;

      for (int n = -1; n < 3; ++n) {
        k1 = BiCubicKernel(dy - (double) n);
        oy2 = oy1 + n;

        if (oy2 < 0) {
          oy2 = 0;
        } else if (oy2 > yMax) {
          oy2 = yMax;
        }

        for (m = -1; m < 3; ++m) {

          k2 = k1 * BiCubicKernel((double) m - dx);
          ox2 = ox1 + m;

          if (ox2 < 0) {
            ox2 = 0;
          } else if (ox2 > xMax) {
            ox2 = xMax;
          }
          red += k2 * qRed(m_image->pixel(ox2, oy2));
          green += k2 * qGreen(m_image->pixel(ox2, oy2));
          blue += k2 * qBlue(m_image->pixel(ox2, oy2));
        }
      }

      image.setPixel(x, y, qRgb(red, green, blue));
    }
  }
  return image;
}
