#include "avsViewer.h"
#include "mywindows.h"
#include "avisynth.h"
#include <conio.h>
#include <QFile>
#include <QPixmap>
#include <iostream>
#include <QLibrary>
#include <QFileInfo>
#include <QTextCodec>
#include <QMessageBox>
#include <QApplication>
#include <QFileDialog>
#include <QStringList>
#include <QTextStream>
#include <QApplication>
#include <QDesktopWidget>
#include <QListWidgetItem>
#include "LocalSocketIpcServer.h"
#include "LocalSocketIpcClient.h"
#include <QWheelEvent>
#include <QFile>
using namespace std;

const QString SEP1 = " ### ";

avsViewer::avsViewer(QWidget *parent, QString path, double mult, bool cutSupport, QString cuts,
    QString ipcID, QString matrix)
    : QWidget(parent), ui(), m_env(NULL), m_inf(), m_clip(), m_frameCount(100), m_current(-1),
        m_currentInput(path), m_version(QString()), m_avsModified(QString()),
        m_inputPath(QString()), m_res(NULL), m_mult(mult), m_currentImage(),
        m_cutSupport(cutSupport), m_dualView(false),
        m_desktopWidth(1920), m_desktopHeight(1080),
        m_ipcID(ipcID), m_currentContent(QString()), m_ipcServer(NULL), m_ipcClient(NULL), m_matrix(matrix)
{
  ui.setupUi(this);
  QString stylepath = QApplication::applicationDirPath()+"/avsViewer.style";
  stylepath = QDir::toNativeSeparators(stylepath);
  if (QFile::exists(stylepath)) {
   QFile file(stylepath);
   if (file.open(QIODevice::ReadOnly)) {
     QString style = file.readAll();
     this->setStyleSheet(style);
   }
  }

  if (!m_cutSupport) {
    delete ui.cutStackedWidget;
    cuts = QString();
  }
  if (m_currentInput.isEmpty()) {
    return;
  }
  ui.showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  delete ui.openAvsPushButton;

  QDesktopWidget *mydesk = QApplication::desktop();
  cout << "Detected desktop resolution: " << mydesk->width() << "x" << mydesk->height() << endl;
  cout << "Detected screen count: " << mydesk->screenCount();
  m_desktopWidth = mydesk->width() / mydesk->screenCount();
  m_desktopHeight = mydesk->height();
  cout << ", using desktop resolution: " << m_desktopWidth << "x" << m_desktopHeight << endl;
  this->init(0, cuts);
}

void avsViewer::wheelEvent(QWheelEvent *event)
{
  double movedby = event->angleDelta().y();
  if (movedby != 0) {
    int numSteps = movedby / 120 + m_current;
    if (numSteps < 0) {
      numSteps = 0;
    }
    if (numSteps >= m_frameCount) {
      numSteps = m_frameCount - 1;
    }
    ui.frameHorizontalSlider->setSliderPosition(numSteps);
    event->accept();
  }
}

avsViewer::~avsViewer()
{
  cout << qPrintable(tr("Closing,...")) << endl;
  if (m_cutSupport) {
    cout << qPrintable(tr("Collecting cuts,..")) << endl;
    QString elem;
    for (int i = 0, c = ui.cutListWidget->count(); i < c; ++i) {
      elem = ui.cutListWidget->item(i)->text();
      if (elem.isEmpty()) {
        continue;
      }
      cout << "Cut: " << qPrintable(elem) << endl;
    }
  }
  if (!m_avsModified.isEmpty()) {
    QFile::remove(m_avsModified);
    cout << qPrintable(tr("deleting: %1").arg(m_avsModified)) << endl;
  }
  cout << qPrintable(tr("finished,...")) << endl;
}

void avsViewer::updateExistingMarks()
{
  if (!m_cutSupport) {
    return;
  }
  QStringList cuts;
  QString elem;
  for (int i = 0, c = ui.cutListWidget->count(); i < c; ++i) {
    elem = ui.cutListWidget->item(i)->text();
    if (elem.isEmpty()) {
      continue;
    }
    cuts << elem;
  }
  ui.frameHorizontalSlider->multiMark(cuts);
}

int avsViewer::invokeImportInternal(const char *inputFile)
{
  try {
    cout << "invokeImportInternal: " << inputFile << std::endl;
    m_res = m_env->Invoke("Import", inputFile); //import current input to environment
    std::cout << "invoke worked" << std::endl;
    return 0;
  } catch (AvisynthError &err) { //catch AvisynthErrors
    cerr << "Avisynth error " << inputFile << ": " << endl << err.msg << endl;
    return -1;
  } catch (...) { //catch the rest
    cerr << "Unknown C++ exception" << endl;
    return -1;
  }
  return 0;
}

int avsViewer::import(const char *inputFile)
{
    cout << "import: " << inputFile << std::endl;
  __try {
    if (invokeImportInternal(inputFile) != 0) {
      return -1;
    }
  }
  __except(1)
  {
    cerr << "-> Win32 exception" << endl;
    return -1;
  }
  return 0;
}

int avsViewer::invokeInternal(const char *function)
{
  try {
    cout << qPrintable(" " + tr("invokeInternal:")) << " " << function << endl;
    m_res = m_env->Invoke(function, AVSValue(&m_res, 1)); //import current input to environment
  } catch (AvisynthError &err) { //catch AvisynthErrors
    cerr << qPrintable(tr("Avisynth error ")) << function << ": " << err.msg << endl;
    return -1;
  } catch (...) { //catch the rest
    cerr << qPrintable(tr("Unknown C++ exception,..")) << endl;
    return -1;
  }
  return 0;
}

int avsViewer::invoke(const char *function)
{
  __try {
    cout << "invoke: " << function << std::endl;
    if (invokeInternal(function) != 0) {
      return -1;
    }
  }
  __except(1)
  {
    cerr << " -> Win32 exception" << endl;
    return -1;
  }
  return 0;
}

//TODO: add cut-edit option
//TODO: add preview-trimms, add reset view

void avsViewer::on_setCutStartPushButton_clicked()
{
  if (!m_cutSupport) {
    return;
  }
  cout << "set cut-start to: " << m_current << endl;
  ui.frameHorizontalSlider->setStart(m_current);
}
void avsViewer::on_setCutEndPushButton_clicked()
{
  if (!m_cutSupport) {
    return;
  }
  cout << "set cut-end to: " << m_current << endl;
  ui.frameHorizontalSlider->setEnd(m_current);
}

void avsViewer::on_jumpBackwardPushButton_clicked()
{
  int frame = m_current;
  if (frame < 0) {
    frame = 0;
  }
  frame -= ui.backwardSpinBox->value();
  if (frame < 0) {
    frame = 0;
  }
  ui.frameHorizontalSlider->setSliderPosition(frame);
}

void avsViewer::on_jumpForwardPushButton_clicked()
{
  int frame = m_current;
  if (frame < 0) {
    frame = 0;
  }
  frame += ui.forwardSpinBox->value();
  if (frame >= m_frameCount) {
    frame = m_frameCount - 1;
  }

  ui.frameHorizontalSlider->setSliderPosition(frame);
}

QString avsViewer::fillUp(int number)
{
  QString frameCount = QString::number(m_frameCount);
  QString ret = QString::number(number);
  while (ret.size() < frameCount.size()) {
    ret = "0" + ret;
  }
  return ret;
}

void avsViewer::on_addCutPushButton_clicked()
{
  if (!m_cutSupport) {
    return;
  }
  int start = ui.frameHorizontalSlider->getStart();
  int end = ui.frameHorizontalSlider->getEnd();
  if ((start == 0 && end == 0) || start == end) {
    return;
  }
  if (start != ui.frameHorizontalSlider->minimum() || end != ui.frameHorizontalSlider->maximum()) {
    if (!this->isValidCut(start, end)) {
      return;
    }
    QString cut = this->fillUp(start) + "-" + this->fillUp(end);
    cout << "add cut item: " << qPrintable(cut) << endl;
    ui.cutListWidget->addItem(cut);
    ui.cutListWidget->sortItems();
    ui.frameHorizontalSlider->resetMarks();
  }
  this->updateExistingMarks();
}

void avsViewer::addCutList(QString list)
{
  if (list.isEmpty()) {
    return;
  }
  QStringList cuts = list.split("#"), elems;
  QString from, to;
  foreach (QString cut, cuts)
  {
    elems = cut.split("-");
    if (elems.count() != 2) {
      continue;
    }
    from = elems.at(0);
    to = elems.at(1);
    if (!this->isValidCut(from.toInt(), to.toInt())) {
      continue;
    }
    ui.cutListWidget->addItem(cut);
    ui.cutListWidget->sortItems();
    ui.frameHorizontalSlider->resetMarks();
  }
  this->updateExistingMarks();
}

bool avsViewer::isValidCut(int start, int end)
{
  cout << qPrintable(" " + tr("checking is valid for: %1-%2").arg(start).arg(end)) << endl;
  if (ui.cutStackedWidget == NULL) {
    cerr << qPrintable(tr("isValidCut called while cutStackedWidget is NULL!")) << endl;
    return false;
  }
  if (ui.cutListWidget == NULL) {
    cerr << qPrintable(tr("isValidCut called while cutListWidget is NULL!")) << endl;
    return false;
  }
  int pos, count;
  QString elem;
  QStringList cutElems;

  if (ui.cutListWidget == NULL) {
    cerr << qPrintable(tr("isValidCut called while cutListWidget is NULL, but was not before!"))
        << endl;
    return false;
  }
  int existingCutCount = ui.cutListWidget->count();

  for (int i = 0; i < existingCutCount; ++i) {
    elem = ui.cutListWidget->item(i)->text();
    if (elem.isEmpty()) {
      continue;
    }
    cutElems = elem.split("-");
    count = cutElems.count();
    if (count != 2) {
      cerr << qPrintable(tr("Wrong formated cut-pair: %1").arg(elem)) << endl;
      return false;
    }
    pos = cutElems.at(0).toInt(); //start
    if ((pos > start) && (pos < end)) {
      cerr
          << qPrintable(
              tr("Ignored %1-%2 since it overlaps with %3(%4).").arg(start).arg(end).arg(elem).arg(
                  pos)) << endl;
      return false;
    }
    pos = cutElems.at(1).toInt(); //end
    if ((pos > start) && (pos < end)) {
      cerr
          << qPrintable(
              tr("Ignored %1-%2 since it overlaps with %3 (%4).").arg(start).arg(end).arg(elem).arg(
                  pos)) << endl;
      return false;
    }
  }
  return true;
}

void avsViewer::on_removeCutPushButton_clicked()
{
  if (!m_cutSupport) {
    return;
  }
  int row = ui.cutListWidget->currentRow();
  if (row == -1) {
    return;
  }
  QListWidgetItem *item = ui.cutListWidget->takeItem(row);
  cout << "removing " << qPrintable(item->text()) << " from cut-list" << endl;
  this->updateExistingMarks();
}

QString removeLastSeparatorFromPath(QString input)
{
  input = input.trimmed();
  if (input.isEmpty()) {
    return input;
  }
  input = QDir::toNativeSeparators(input);
  int size = input.size();
  if (!input.endsWith(QDir::separator())) {
    return input;
  } else if (size == 1) { //input only consists of the separator
    return QString();
  }
  return input.remove(size - 1, 1);
}

QString getDirectory(const QString input)
{
  if (input.isEmpty()) {
    return QString();
  }
  QString path = input;
  QFileInfo info(path);
  if (info.isDir()) {
    return removeLastSeparatorFromPath(path);
  }
  QString output = path;
  output = output.replace("\\", "/");
  int index = output.lastIndexOf("/");
  if (index == -1) {
    return QString();
  }
  output = output.remove(index, output.size());
  return QDir::toNativeSeparators(output);
}

void avsViewer::on_saveImagePushButton_clicked()
{
  ui.showLabel->setText(tr("Set output png file,.."));
  QString name = tr("Select input file");
  QString select = tr("Output (*.png)");
  QString inputPath = QApplication::applicationDirPath();
  if (!m_inputPath.isEmpty()) {
    inputPath = m_inputPath;
  }
  QString input = QFileDialog::getSaveFileName(this, name, inputPath, select);
  if (input.isEmpty()) {
    return;
  }
  m_inputPath = getDirectory(input);
  if (!m_currentImage.save(input, "PNG")) {
    QMessageBox::warning(this, "Error", tr("Couldn't save %1").arg(input));
  }
}

void avsViewer::cleanUp()
{
  if (m_env != NULL) {
    cout << "Clean up old script environment,.." << endl;
    m_res = 0;
    m_clip = 0;
    m_env->DeleteScriptEnvironment(); //delete the old script environment
    m_env = NULL; // ensure new environment created next time
    m_current = 0;
    m_cuts = QString();
  }
}

void avsViewer::on_infoCheckBox_toggled()
{
  this->cleanUp();
  this->init(m_current);
}

void avsViewer::on_histogramCheckBox_toggled()
{
  this->cleanUp();
  this->init(m_current);
}

void avsViewer::on_aspectRatioAdjustmentComboBox_currentIndexChanged(QString value)
{
  Q_UNUSED(value);
  this->cleanUp();
  this->init(m_current);
}

void avsViewer::fromConsoleReader(QString text)
{
  cout << "From console reader: " << qPrintable(text) << endl;
}

QString removeQuotes(QString input)
{
  QString ret = input.trimmed();
  if (ret.startsWith("\"")) {
    ret = ret.remove(0, 1);
  }
  if (ret.endsWith("\"")) {
    ret = ret.remove(ret.size() - 1, 1);
  }
  return ret;
}

QString getWholeFileName(const QString input)
{
  if (input.isEmpty()) {
    return QString();
  }
  QString output = QDir::toNativeSeparators(input);
  int index = output.lastIndexOf(QDir::separator());
  if (output.endsWith(QDir::separator())) {
    return QString();
  } else if (index != -1) {
    output = output.remove(0, index + 1);
  }
  output = removeQuotes(output);
  return QDir::toNativeSeparators(output);
}

QString getFileName(const QString input)
{
  if (input.isEmpty()) {
    return QString();
  }
  QString output = getWholeFileName(input);
  int index = output.lastIndexOf(".");
  if (index != -1) {
    output = output.remove(index, output.size());
  }
  return output;
}

int saveTextTo(QString text, QString to)
{
  if (text.isEmpty()) {
    return -1;
  }
  QFile file(to);
  file.remove();
  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    out.setCodec("System");
    out << text;
    if (file.exists()) {
      file.close();
      return 0;
    }
  }
  return -1;
}

void checkInputType(const QString content, bool &ffmpegSource, bool &mpeg2source, bool &dgnvsource,
    QString &ffms2Line)
{
  bool ffms2Avs = false;
  foreach(QString line, content.split("\n"))
  {
    if (line.contains("FFMpegSource2(", Qt::CaseInsensitive)
        || line.contains("FFVideoSource(", Qt::CaseInsensitive)) {
      ffmpegSource = true;
    }
    if (line.contains("MPEG2Source(", Qt::CaseInsensitive)) {
      mpeg2source = true;
    }
    if (line.contains("DGSource(", Qt::CaseInsensitive)) {
      dgnvsource = true;
    }
    if (line.contains("ffms2.dll", Qt::CaseInsensitive)) {
      ffms2Line = line;
      ffms2Line = ffms2Line.remove(0, ffms2Line.indexOf("\"") + 1);
      ffms2Line = ffms2Line.remove(ffms2Line.indexOf("\""), ffms2Line.size());
      ffms2Line = getDirectory(ffms2Line);
      ffms2Line += QDir::separator();
      ffms2Line += "FFMS2.avsi";
      ffms2Line = QDir::toNativeSeparators(ffms2Line);
    }
    if (line.contains("FFMS2.avs", Qt::CaseInsensitive)) {
      ffms2Avs = true;
    }
  }
  cout << " Input type:" << endl;
  cout << "  ffmpegSource: " << ffmpegSource << endl;
  cout << "  mpeg2source: " << mpeg2source << endl;
  cout << "  dgnvsource: " << dgnvsource << endl;
  cout << "  ffmpegsource2: " << ffms2Avs << endl;
}

void addShowInfoToContent(const int distributorIndex, const bool ffmpegSource,
    const QString content, QString &newContent, const bool ffms2Avs, const QString ffms2Line,
    bool &invokeFFInfo, const bool mpeg2source, const bool dgnvsource)
{
  if (ffmpegSource) {
    if (distributorIndex != -1) { // contains distributor
      newContent = content;
      newContent = newContent.remove(distributorIndex, newContent.size()).trimmed();
      if (!ffms2Avs && !ffms2Line.isEmpty()) {
        newContent += "\n";
        newContent += "Import(\"" + ffms2Line + "\")";
      }
      newContent += "\n";
      newContent += "SetMTMode(5)";
      newContent += "\n";
      newContent += "FFInfo()";
      newContent += "\n";
      newContent += "distributor()";
      newContent += "\n";
      newContent += "return last";
    } else if (!ffms2Avs && !ffms2Line.isEmpty()) {
      newContent = content;
      int index = newContent.lastIndexOf("return");
      if (index != -1) {
        newContent = newContent.remove(index, newContent.size()).trimmed();
      }
      if (content.contains("SetModeMT(")) {
        newContent += "\n";
        newContent += "SeMTMode(5)";
      }
      newContent += "\n";
      newContent += "Import(\"" + ffms2Line + "\")";
      newContent += "\n";
      newContent += "FFInfo()";
      newContent += "\n";
      newContent += "return last";
    } else {
      invokeFFInfo = true;
    }
  } else if (mpeg2source) {
    newContent = content;
    newContent = newContent.replace(".d2v\"", ".d2v\", info=1", Qt::CaseInsensitive);
  } else if (dgnvsource) {
    newContent = content;
    cout << " replacing '.dgi\"'' with '.dgi\", debug=true'" << endl;
    newContent = newContent.replace(".dgi\"", ".dgi\", debug=true", Qt::CaseInsensitive);
  } else if (distributorIndex != -1) { // contains distributor
      newContent = content;
      newContent = newContent.remove(distributorIndex, newContent.size()).trimmed();
      newContent += "\n";
      newContent += "Info()";
      newContent += "\n";
      newContent += "return last";
  } else {
      int index = newContent.lastIndexOf("return");
      if (index != -1) {
        newContent = newContent.remove(index, newContent.size()).trimmed();
      }
      newContent += "Info()";
      newContent += "\n";
      newContent += "return last";}
}

void addHistrogramToContent(const QString content, QString &newContent, const QString& matrix)
{
  cout << "Calling histogram with matrix " << qPrintable(matrix) << endl;
  if (newContent.isEmpty()) {
    newContent = content;
  }
  int index1 = newContent.indexOf("# adjust color to RGB32");
  int index = newContent.indexOf("StackHorizontal(Source, SourceFiltered)");
  if (index == -1 && index1 != -1) { // no split view
      if (matrix.isEmpty()) {
        newContent.insert(index1,"\nConvertToYV12().ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n");
      } else {
        newContent.insert(index1,"\nConvertToYV12(matrix=\""+matrix+"\").ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n");
      }
    return;
  } else if (index != -1) {
    if (matrix.isEmpty()) {
      newContent = newContent.replace("StackHorizontal(Source, SourceFiltered", "StackHorizontal(Source, SourceFiltered.ConvertToYV12().ColorYUV(analyze=true).Histogram(mode=\"levels\")");
      newContent = newContent.replace("StackHorizontal(Source", "StackHorizontal(Source.ConvertToYV12().ColorYUV(analyze=true).Histogram(mode=\"levels\")");
    } else {
      newContent = newContent.replace("StackHorizontal(Source, SourceFiltered", "StackHorizontal(Source, SourceFiltered.ConvertToYV12(matrix=\""+matrix+"\").ColorYUV(analyze=true).Histogram(mode=\"levels\")");
      newContent = newContent.replace("StackHorizontal(Source", "StackHorizontal(Source.ConvertToYV12(matrix=\""+matrix+"\").ColorYUV(analyze=true).Histogram(mode=\"levels\")");
    }

  } else {
    index = newContent.indexOf("distributor()", Qt::CaseInsensitive);
    if (index != -1) {
      if (matrix.isEmpty()) {
        newContent.insert(index, "\nConvertToYV12().ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n");
      } else {
        newContent.insert(index, "\nConvertToYV12(matrix=\""+matrix+"\").ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n");
      }
      return;
    }
    index = newContent.indexOf("return", Qt::CaseInsensitive);
    if (index != -1) {
      if (matrix.isEmpty()) {
        newContent.insert(index, "\nConvertToYV12().ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n");
      } else {
        newContent.insert(index, "\nConvertToYV12(matrix=\""+matrix+"\").ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n");
      }
      return;
    }
    if (matrix.isEmpty()) {
      newContent += "\nConvertToYV12().ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n";
    } else {
      newContent += "\nConvertToYV12(matrix=\""+matrix+"\").ColorYUV(analyze=true)\nHistogram(mode=\"levels\")\n";
    }
  }
}

void applyResolution(const QString content, QString &newContent, double mult, QString resize)
{
  cout << "applyResolution: " << qPrintable(resize) << ", mult: " << mult << endl;
  if (resize == "None") {
    return;
  }
  if (newContent.isEmpty()) {
    newContent = content;
  }
  newContent = newContent.trimmed();
  int index = newContent.indexOf("distributor()", Qt::CaseInsensitive);
  cout << "index of 'distributor()': " << index << endl;
  QString resizer;
  if (mult != 0 && mult != 1) {
    resizer = resize + "Resize(Ceil(last.Width*" + QString::number(mult) + " -(Ceil(last.Width*" + QString::number(mult) + ") % 4)), last.Height)\n";
  } else {
    resizer = resize + "Resize(last.Width-last.Width% 4, last.Height)\n";
  }
  if (index != -1) {
    newContent.insert(index, resizer);
    return;
  }
  QStringList lines = newContent.split("\n");
  QString lastLine = lines.last().trimmed();
  if (lastLine.startsWith("return", Qt::CaseInsensitive)) {
    newContent += "." + resizer;
    return;
  }
  newContent += "\nlast." + resizer;
}

void avsViewer::receivedMessage(const QString& message)
{
  cout << "got message: " << qPrintable(message) << endl;
  this->setWindowTitle(message);
  QStringList typeAndValue = message.split(SEP1);
  switch (typeAndValue.count())
  {
    case 3 :
      this->callMethod(typeAndValue.at(0), typeAndValue.at(1), typeAndValue.at(2));
      break;
    case 2 :
      this->callMethod(typeAndValue.at(0), typeAndValue.at(1), QString());
      break;
    default :
      cout << qPrintable(tr("ignoring received message: %1").arg(message)) << endl;
      break;
  }
}

QString getCurrentInput(const QString& script)
{
  if (script.isEmpty()) {
    return QString();
  }
  // # loading source: F:\TestClips&Co\test.avi
  QString element = "# loading source: ";
  int index = script.indexOf(element);
  if (index == -1) {
    return QString();
  }
  QString input = script;
  input = input.remove(0, index + element.size());
  input = input.remove(input.indexOf("\n"), input.size());
  return input;
}

void avsViewer::callMethod(const QString& typ, const QString& value, const QString &input)
{
  cout << "callmethod: " << qPrintable(typ) << ", value "<< qPrintable(value) << ", input " << qPrintable(input) << std::endl;
  this->setWindowTitle(QString("%1, %2:\n%3").arg(typ).arg(value).arg(input));
  if (typ == "changeTo" && QFile::exists(value)) {
    int currentPosition = 0;
    QString cuts;
    QString currentInput = getCurrentInput(m_currentContent); // the input of the avisynth script
    if (currentInput == input) {
      currentPosition = m_current;
      cuts = m_cuts;
    }
    this->killEnv();
    m_currentInput = value; //set current input
    cout << "Change current input to: " << qPrintable(m_currentInput) << endl;
    ui.showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
    this->init(currentPosition, cuts);
    qApp->processEvents();
    return;
  }
  cerr << "unsupported typ: " << qPrintable(typ) << endl;
  cerr << "     with value: " << qPrintable(value) << endl;
}

/**
 * initilazing an avisynth environment for the current input file
 **/
int avsViewer::init(int start, const QString cuts)
{
  if (start < 0) {
    start = 0;
  }
  if (m_ipcID != QString()) {
    if (m_ipcServer == NULL) {
      cout << " starting ipc server, with serverName " << qPrintable(m_ipcID + "AVSVIEWER") << endl;
      m_ipcServer = new LocalSocketIpcServer(m_ipcID + "AVSVIEWER", this);
      connect(m_ipcServer, SIGNAL(messageReceived(QString)), this, SLOT(receivedMessage(QString)));
    }
    if (m_ipcClient == NULL) {
      cout << " starting ipc client with serverName " << qPrintable(m_ipcID + "HYBRID") << endl;
      m_ipcClient = new LocalSocketIpcClient(m_ipcID + "HYBRID", this);
    }
  }

  m_current = -1; //reset frameIndex
  cout << qPrintable(tr("Initializing the avisynth script environment,..")) << endl;
  if (m_currentInput.isEmpty()) {
    cerr << qPrintable(tr("Current input is empty,..")) << endl;
    return -1;
  }
  if (m_env != NULL) { //if I do not abort here application will crash on 'm_res.AsClip()' later
    cerr << qPrintable(tr("Init called on existing environment,..")) << endl;
    return -2;
  }
  bool firstTime = this->minimumSize().width() == 0;
  try { // load script
    QLibrary avsDLL("avisynth.dll");
    if (!avsDLL.isLoaded() && !avsDLL.load()) { //load avisynth.dll if it's not already loaded and abort if it couldn't be loaded
      QString error = avsDLL.errorString();
      if (!error.isEmpty()) {
        cerr << qPrintable(tr("Could not load avisynth.dll!")) << qPrintable(error) << endl;
        return -3;
      }
      cerr << qPrintable(tr("Could not load avisynth.dll!")) << endl;
      return -4;
    }
    cout << qPrintable(" " + tr("Loaded avisynth dll,..")) << endl;
    IScriptEnvironment* (*CreateScriptEnvironment)(
        int version) = (IScriptEnvironment*(*)(int)) avsDLL.resolve("CreateScriptEnvironment"); //resolve CreateScriptEnvironment from the dll
    cout << qPrintable(" " + tr("Loaded CreateScriptEnvironment definition from dll,..")) << endl;
    m_env = CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION); //create a new IScriptEnvironment
    if (!m_env) { //abort if IScriptEnvironment couldn't be created
      cerr << qPrintable(tr("Could not create IScriptenvironment,...")) << endl;
      return -4;
    }
    cout << qPrintable(" " + tr("Created an IScriptEnvironment,..")) << endl;
    try {
      cout << qPrintable(" " + tr("Looking for avisynth version,..")) << endl;
      AVSValue as_version;
      as_version = m_env->Invoke("VersionString", AVSValue(&as_version, 0)); //get current version info
      m_version = as_version.AsString(); //save current version for later use
      cout << qPrintable("  " + tr("current avisynth version: %1").arg(m_version)) << endl;
    } catch (...) {
      cerr << qPrintable(tr("Could not get the current avisynth version,..")) << endl;
      return -5;
    }
    QString newContent, input = m_currentInput;
    bool invokeFFInfo = false;
    QFile file(input);
    bool ffmpegSource = false;
    bool showInfo = false;
    bool mpeg2source = false;
    bool dgnvsource = false;
    bool ffms2Avs = false;
    if (file.open(QIODevice::ReadOnly)) {
      QString content = file.readAll(), ffms2Line;
      QStringList lines = content.split("\n");
      QStringList nocomments;
      foreach(QString line, lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
          continue;
        }
        nocomments << line;
      }
      content = lines.join("\n");
      file.close();
      m_currentContent = content;
      m_dualView = content.contains("SourceFiltered = Source");
      checkInputType(content, ffmpegSource, mpeg2source, dgnvsource, ffms2Line);
      bool mpeg2info = mpeg2source;
      if (mpeg2info && content.contains("info=3")) {
        mpeg2info = false;
      }
      ui.infoCheckBox->setEnabled(true);
      showInfo = ui.infoCheckBox->isChecked();
      if (showInfo) {
        int index = content.indexOf("distributor()", Qt::CaseInsensitive);
        addShowInfoToContent(index, ffmpegSource, content, newContent, ffms2Avs, ffms2Line,
            invokeFFInfo, mpeg2source, dgnvsource);
      }
      if (ui.histogramCheckBox->isChecked()) {
        addHistrogramToContent(content, newContent, m_matrix);
      }
      applyResolution(content, newContent, m_mult, ui.aspectRatioAdjustmentComboBox->currentText());
    } else {
      cerr << qPrintable(tr("Couldn't read content of: %1").arg(input)) << endl;
      return -1;
    }
    if (!newContent.isEmpty()) { //create new modfied avs file
      QString directory = getDirectory(m_currentInput);
      QString name = getFileName(m_currentInput);
      m_avsModified = QDir::toNativeSeparators(directory + QDir::separator() + name + "_tmp.avs");
      if (saveTextTo(newContent, m_avsModified) == 0) {
        input = m_avsModified;
      }
    }

    cout << qPrintable(" " + tr("Importing: %1 into the environment,.. ").arg(input)) << endl;
    const char* infile = input.toLocal8Bit(); //convert input name to char*
    cout << "importing " << infile << std::endl;
    int ret = import(infile);
    if (ret != 0) {
      cerr << qPrintable(tr("Couldn't import %2 (1): %1").arg(infile).arg(ret)) << endl;
      return -6;
    }
    if (!m_res.Defined()) {
      QString error = tr("Couldn't import (2):") + " " + input;
      error += "\r\n";
      error += tr("Script seems not to be a valid avisynth script.");
      cerr << qPrintable(error) << endl;
      return -7;
    }
    cout << qPrintable(" " + tr("Script seems to be a valid avisynth script.")) << endl;
    cout << qPrintable(" " + tr("Initializating a clip,..")) << endl;
    m_clip = m_res.AsClip(); //get clip
    cout << qPrintable(" " + tr("Grabbing clip infos,..")) << endl;
    m_inf = m_clip->GetVideoInfo(); //get clip infos
    if (!m_inf.HasVideo()) { //abort if clip has no video
      cerr << qPrintable(tr("Input has no video stream -> aborting")) << endl;
      return -8;
    }
    cout << qPrintable(" " + tr("Checking colorspace,..")) << endl;
    bool reload = false;
    if (m_inf.IsRGB32()) {
      cout << qPrintable("  " + tr("current color space is RGB32")) << endl;
    } else {
      if (m_inf.IsRGB()) {
        cout << qPrintable("  " + tr("current color space is RGB")) << endl;
      } else if (m_inf.IsYV12()) {
        cout << qPrintable("  " + tr("current color space is Yv12")) << endl;
      } else if (m_inf.IsRGB24()) {
        cout << qPrintable("  " + tr("current color space is RGB24")) << endl;
      } else if (m_inf.IsRGB32()) {
        cout << qPrintable("  " + tr("current color space is RGB32")) << endl;
      } else if (m_inf.IsYUY2()) {
        cout << qPrintable("  " + tr("current color space is YUY2")) << endl;
      } else if (m_inf.IsYUV()) {
        cout << qPrintable("  " + tr("current color space is YUV")) << endl;
      } else {
        cout << qPrintable("  " + tr("current color space is unknown")) << endl;
      }
      if (this->invoke("ConvertToRGB32") != 0) {
        cerr << qPrintable(tr("Couldn't invoke 'ConvertToTGB()' -> aborting")) << endl;
        this->killEnv();
        return -9;
      }
      reload = true;
    }
    if (invokeFFInfo) {
      if (this->invoke("FFInfo") != 0) {
        cerr << qPrintable(tr("Couldn't invoke 'FFInfo()' -> aborting")) << endl;
        this->killEnv();
        return -10;
      }
      reload = true;
    }
    if (reload) {
      cout << qPrintable(" " + tr("initializating the clip anew,..")) << endl;
      m_clip = m_res.AsClip(); // update clip
      cout << qPrintable(" " + tr("Grabbing clip infos,..")) << endl;
      m_inf = m_clip->GetVideoInfo(); // update clip info
    }
    cout << qPrintable(" " + tr("Grabbing clip length,..")) << endl;
    m_frameCount = m_inf.num_frames; //get frame count
    cout << qPrintable(" -> " + tr("Clip contains: %1 frames").arg(m_frameCount)) << endl;
    cout << qPrintable(" " + tr("Adjusting slider to frame count,..")) << endl;
    ui.frameHorizontalSlider->setMaximum(m_frameCount);
    ui.frameHorizontalSlider->resetMarks();
    int width = m_inf.width;
    int height = m_inf.height;
    while (width > (m_desktopWidth - 10) || height > (m_desktopHeight - 10)) {
      width *= 0.9;
      height *= 0.9;
    }
    cout << qPrintable(" " + tr("label resolution %1x%2").arg(width).arg(height)) << endl;
    ui.showLabel->setMaximumSize(32767, 32767);
    if (firstTime || ui.histogramCheckBox->isChecked()) {
      ui.showLabel->resize(width, height);
    }
    cout << qPrintable(" " + tr("showing first frame,..")) << endl;
    this->showFrame(start); //show first frame
  } catch (AvisynthError &err) { //catch AvisynthErrors
    cerr << qPrintable(tr("-> Avisynth error: %1").arg(err.msg)) << endl;
    return -11;
  } catch (...) { //catch everything else
    cerr << qPrintable("->" + tr("Unknown error")) << endl;
    return -12;
  }
  cout << qPrintable(tr("finished initializing the avisynth script environment,..")) << endl;
  if (m_ipcClient != NULL) {
    m_ipcClient->send_MessageToServer("AvsViewer started ipcClient&Server with id " + m_ipcID);
  }
  if (!cuts.isEmpty()) {
    this->addCutList(cuts);
  }
  m_cuts = cuts;
  if ((!firstTime && !ui.histogramCheckBox->isChecked()) || this->isFullScreen()) {
    return 0;
  }
  QSize size = ui.showLabel->size();
  int width = size.width() + 8;
  int height = size.height() + 48;
  cout << qPrintable(" " + tr("window resolution %1x%2").arg(width).arg(height)) << endl;
  this->resize(width, height);

  return 0;
}
/**
 * adjusts frame-index and frame to slider position
 **/
void avsViewer::on_frameHorizontalSlider_valueChanged(int value)
{
  cout << "slider changed to: " << value << endl;
  ui.frameNumberLabel->setText("(" + QString::number(value) + ")"); // update frame label
  if (!ui.frameHorizontalSlider->isSliderDown()) {
    this->showFrame(value); //show current frame
  }
}

/**
 * shows frame number i
 **/
void avsViewer::showFrame(int i)
{
  cout << " showFrame: " << i << endl;
  cout << "  m_frameCount: " << m_frameCount << endl;
  cout << "  m_env: " << int(m_env != NULL) << endl;
  if (m_env == NULL || i > m_frameCount) {
    return;
  }

  try {
    PVideoFrame f = m_clip->GetFrame(i, m_env); // get frame number i
    if (m_mult == 0) {
      m_mult = 1;
    }
    int width = m_inf.width;
    int height = m_inf.height;
    cout << "frame resolution: " << width << "x" << height << endl;
    QImage image(f->GetReadPtr(), width, height, QImage::Format_RGB32); //create a QImage
    ui.showLabel->setText(QString());
    m_currentImage = image.mirrored(); // flip image otherwise it's upside down

    ui.showLabel->setPixmap(QPixmap::fromImage(m_currentImage)); // addResolution does the resizing
    //cout << "show frame: " << i << endl;
    m_current = i; //set m_current to i
    ui.frameHorizontalSlider->setSliderPosition(m_current); // adjust the slider position
    QString title = tr("showing frame number: %1 of %2").arg(m_current).arg(m_frameCount); //adjust title bar;
    if (m_dualView) {
      title += " " + tr("(left side = original, right side = filtered)");
    }
    //this->setWindowTitle(title);
  } catch (...) {
    cerr << " couldn't show frame,..." << endl;
  }
}

void avsViewer::killEnv()
{
  this->cleanUp();
  if (!m_avsModified.isEmpty()) {
    QFile::remove(m_avsModified);
    m_avsModified = QString();
  }
  ui.frameHorizontalSlider->resetMarks();
  if (m_cutSupport) {
    ui.cutListWidget->clear();
  }
}

/**
 * allows to select a .avs file, starts the initialization
 **/
void avsViewer::on_openAvsPushButton_clicked()
{
  ui.showLabel->setText(tr("Opening new file,.."));
  QString name = tr("Select input file");
  QString select = tr("Input (*.avs)");
  QString inputPath = QApplication::applicationDirPath();
  QString input = QFileDialog::getOpenFileName(this, name, inputPath, select);
  if (!input.endsWith(".avs") || input.isEmpty()) { //abort if input does not end with .avs
    cout << "Current input is empty or not an .avs file,.." << endl;
    return;
  }
  this->killEnv();

  m_currentInput = input; //set current input
  ui.showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  cout << "Current input: " << qPrintable(m_currentInput) << endl;
  this->init();
}

/**
 * shows the next frame
 **/
void avsViewer::on_nextPushButton_clicked()
{
  if (m_current < 0) {
    m_current = 0;
  }
  this->showFrame(m_current + 1); // show next frame
}
/**
 * shows the previous frame
 **/
void avsViewer::on_previousPushButton_clicked()
{
  if (m_current < 1) {
    m_current = 1;
  }
  this->showFrame(m_current - 1); // show previous frame
}
/**
 * shows the frame of the index where the slider was released
 **/
void avsViewer::on_frameHorizontalSlider_sliderReleased()
{
  this->showFrame(ui.frameHorizontalSlider->sliderPosition()); // show frame for current slider position
}
