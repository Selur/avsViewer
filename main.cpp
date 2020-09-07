#include "avsViewer.h"

#include <QtGui>
#include <QApplication>
#include <QStringList>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  QString path;
  double mult = 0;
  QString ipcID;
  QString matrix;
  QString option;
  for(int i=0; i < argc; ++i) {
    option = QString(argv[i]);
    if (option == "--input" && i+1 <= argc) {
      path = QString(argv[++i]);
      if (!path.endsWith(".avs", Qt::CaseInsensitive)) {
        cout << qPrintable(QObject::tr("Input has no .avs extension,..")) << endl;
        return -1;
      }
      continue;
    }
    if (option == "--aspect" && i+1 <= argc) {
      mult = QString(argv[++i]).toDouble();
      cout << qPrintable(QObject::tr("aspect mult: %1").arg(mult)) << endl;
      continue;
    }
    if (option == "--listen" && i+1 <= argc) {
      ipcID = QString(argv[++i]);
      cout << qPrintable(QObject::tr("server id: %1").arg(ipcID)) << endl;
      continue;
    }
    if (option == "--matrix") {
      matrix = QString(argv[++i]);
      cout << qPrintable(QObject::tr("matrix: %1").arg(matrix)) << endl;
      continue;
    }
  }
  avsViewer w(0, path, mult, ipcID, matrix);
  w.setAttribute(Qt::WA_QuitOnClose);
  w.show();
  return a.exec();
}
