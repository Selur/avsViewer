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
  // start at 1 (argv[0] is the program path) and use i+1 < argc, since argv[argc] is the terminating null
  for(int i=1; i < argc; ++i) {
    option = QString(argv[i]);
    if (option == "--input") {
      if (i+1 >= argc) {
        cout << qPrintable(QObject::tr("--input needs a file name,..")) << endl;
        return -1;
      }
      path = QString(argv[++i]);
      if (!path.endsWith(".avs", Qt::CaseInsensitive)) {
        cout << qPrintable(QObject::tr("Input has no .avs extension,..")) << endl;
        return -1;
      }
      continue;
    }
    if (option == "--aspect" && i+1 < argc) {
      mult = QString(argv[++i]).toDouble();
      cout << qPrintable(QObject::tr("aspect mult: %1").arg(mult)) << endl;
      continue;
    }
    if (option == "--listen" && i+1 < argc) {
      ipcID = QString(argv[++i]);
      cout << qPrintable(QObject::tr("server id: %1").arg(ipcID)) << endl;
      continue;
    }
    if (option == "--matrix" && i+1 < argc) {
      matrix = QString(argv[++i]);
      cout << qPrintable(QObject::tr("matrix: %1").arg(matrix)) << endl;
      continue;
    }
    cout << qPrintable(QObject::tr("ignoring unknown or incomplete option: %1").arg(option)) << endl;
  }
  avsViewer w(0, path, mult, ipcID, matrix);
  w.setAttribute(Qt::WA_QuitOnClose);
  w.show();
  return a.exec();
}
