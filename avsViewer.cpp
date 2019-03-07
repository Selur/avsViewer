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
#include <QListWidgetItem>
#include "LocalSocketIpcServer.h"
#include "LocalSocketIpcClient.h"
#include <QWheelEvent>
#include <QFile>
#include <QScrollBar>
#include <QScreen>
using namespace std;

const QString SEP1 = " ### ";

avsViewer::avsViewer(QWidget *parent, const QString& path, const double& mult, const QString& ipcID, const QString& matrix)
    : QWidget(parent), ui(), m_env(nullptr), m_inf(), m_clip(), m_frameCount(100), m_current(-1),
        m_currentInput(path), m_version(QString()), m_avsModified(QString()),
        m_inputPath(QString()), m_res(NULL), m_mult(mult), m_currentImage(),
        m_dualView(false), m_desktopWidth(1920), m_desktopHeight(1080),
        m_ipcID(ipcID), m_currentContent(QString()), m_ipcServer(nullptr), m_ipcClient(nullptr),
        m_matrix(matrix), m_showLabel(new QLabel()), m_zoom(1),
        m_currentFrameWidth(0), m_currentFrameHeight(0), m_fill(0), m_noAddBorders(false)
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
  if (m_currentInput.isEmpty()) {
    return;
  }
  m_showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  ui.scrollArea->setWidget(m_showLabel);
  delete ui.openAvsPushButton;

  QScreen *screen = QGuiApplication::primaryScreen();
  QRect  screenGeometry = screen->geometry();
  int height = screenGeometry.height();
  int width = screenGeometry.width();
  m_desktopWidth = width;
  m_desktopHeight = height;
  cout << "-> using desktop resolution: " << m_desktopWidth << "x" << m_desktopHeight << endl;
  this->init(0);
}

void avsViewer::wheelEvent(QWheelEvent *event)
{
  double movedby = event->angleDelta().y();
  if (movedby != 0.0) {
    int numSteps = int (movedby / 120 + m_current);
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
  if (!m_avsModified.isEmpty()) {
    QFile::remove(m_avsModified);
    cout << qPrintable(tr("deleting: %1").arg(m_avsModified)) << endl;
  }
  cout << qPrintable(tr("finished,...")) << endl;
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
}

int avsViewer::import(const char *inputFile)
{
  cout << "import: " << inputFile << std::endl;
  __try  {
    if (invokeImportInternal(inputFile) != 0) {
      return -1;
    }
    return 0;
  }
  __except(1) {
    cerr << "-> Win32 exception" << endl;
    return -1;
  }
}

int avsViewer::invokeInternal(const char *function)
{
  try {
    cout << qPrintable(tr("invokeInternal: ")) << function << endl;
    m_res = m_env->Invoke(function, AVSValue(&m_res, 1)); //import current input to environment
    return 0;
  } catch (AvisynthError &err) { //catch AvisynthErrors
    cerr << qPrintable(tr("Avisynth error ")) << function << ": " << err.msg << endl;
    return -1;
  } catch (...) { //catch the rest
    cerr << qPrintable(tr("Unknown C++ exception,..")) << endl;
    return -1;
  }
}

int avsViewer::invoke(const char *function)
{
  __try {
    cout << "invoke: " << function << std::endl;
    if (invokeInternal(function) != 0) {
      return -1;
    }
    return 0;
  }
  __except(1) {
    cerr << " -> Win32 exception" << endl;
    return -1;
  }

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

QString getDirectory(const QString& input)
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
  m_showLabel->setText(tr("Set output png file,.."));
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
  if (!m_currentImage.save(input, "PNG", 100)) {
    QMessageBox::warning(this, "Error", tr("Couldn't save %1").arg(input));
  }
  this->showFrame(m_current);
}

void avsViewer::cleanUp()
{
  if (m_env != nullptr) {
    cout << "Clean up old script environment,.." << endl;
    m_res = 0;
    m_clip = nullptr;
    m_env->DeleteScriptEnvironment(); //delete the old script environment
    m_env = nullptr; // ensure new environment created next time
    m_current = 0;
    m_fill = 0;
    m_noAddBorders = false;
    qApp->processEvents();
  }
}

void avsViewer::refresh()
{
  int curr = m_current;
  this->cleanUp();
  m_current = curr;
  this->init(m_current);
}

void avsViewer::on_infoCheckBox_toggled()
{
  this->refresh();
}

void avsViewer::on_histogramCheckBox_toggled()
{
  this->refresh();
}


void avsViewer::on_scrollingCheckBox_toggled()
{
  this->refresh();
}

void avsViewer::on_aspectRatioAdjustmentComboBox_currentIndexChanged(const QString& value)
{
  Q_UNUSED(value);
  this->refresh();
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

QString getWholeFileName(const QString& input)
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

QString getFileName(const QString& input)
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

int saveTextTo(const QString& text, const QString& to)
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

void checkInputType(const QString& content, bool &ffmpegSource, bool &mpeg2source, bool &dgnvsource, QString &ffms2Line)
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
    const QString& content, QString &newContent, const QString& ffms2Line,
    bool &invokeFFInfo, const bool mpeg2source, const bool dgnvsource)
{
  if (ffmpegSource) {
    if (distributorIndex != -1) { // contains distributor
      newContent = content;
      newContent = newContent.remove(distributorIndex, newContent.size()).trimmed();
      if (!ffms2Line.isEmpty()) {
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
    } else if (!ffms2Line.isEmpty()) {
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
      newContent = content;
      int index = newContent.lastIndexOf("return");
      if (index != -1) {
        newContent = newContent.remove(index, newContent.size()).trimmed();
      }
      newContent += "Info()";
      newContent += "\n";
      newContent += "return last";}
}

void addHistrogramToContent(const QString& content, QString &newContent, const QString& matrix)
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

void applyResolution(const QString& content, QString &newContent, double mult, const QString& resize)
{
  if (resize == QString("None")) {
    return;
  }
  if (mult == 0.0 || mult == 1.0) {
     return;
  }
  if (newContent.isEmpty()) {
    newContent = content;
  }
  newContent = newContent.trimmed();
  QString resizer;
  resizer = resize + "Resize(Ceil(last.Width*" + QString::number(mult) + "), last.Height)\n";
  int index = newContent.indexOf("distributor()", Qt::CaseInsensitive);
  if (index != -1) { // add before distributor
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
  if (message.isEmpty()) {
    return;
  }
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

void avsViewer::changeTo(const QString& input, const QString& value)
{
  int currentPosition = 0;
  QString currentInput = getCurrentInput(m_currentContent); // the input of the avisynth script
  QFile file(value);
  QString newContent;
  if (file.open(QIODevice::ReadOnly)) {
    newContent = file.readAll();
    file.close();
  }
  QString newInput = getCurrentInput(newContent); // the input of the avisynth script
  if (newInput.isEmpty()) {
    newInput = input;
  }
  if (currentInput == newInput) { // input didn't change keeping position
    currentPosition = m_current;
  }
  if (m_currentFrameWidth == 0) {
    m_currentFrameWidth = m_inf.width;
    m_currentFrameHeight = m_inf.height;
  }

  this->killEnv(); // killing old Avisynth environment
  m_currentInput = value; //set current input
  m_showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  this->init(currentPosition);
}

void avsViewer::callMethod(const QString& typ, const QString& value, const QString &input)
{
  cout << "callmethod: " << qPrintable(typ) << ", value "<< qPrintable(value) << ", input " << qPrintable(input) << std::endl;
  if (!QFile::exists(value)){
    cout << qPrintable(QString("Change ignored since '%1' doesn't exist.").arg(value)) << std::endl;
    return;
  }
  this->setWindowTitle(QString("%1, %2:\n%3").arg(typ).arg(value).arg(input));
  if (typ == "changeTo") {
    this->changeTo(input, value);
    return;
  }
  cerr << "unsupported typ: " << qPrintable(typ) << endl;
  cerr << "     with value: " << qPrintable(value) << endl;
}

/**
 * initilazing an avisynth environment for the current input file
 **/
int avsViewer::init(int start)
{
  if (start < 0) {
    start = 0;
  }
  if (m_ipcID != QString()) {
    if (m_ipcServer == nullptr) {
      cout << " starting ipc server, with serverName " << qPrintable(m_ipcID + "AVSVIEWER") << endl;
      m_ipcServer = new LocalSocketIpcServer(m_ipcID + "AVSVIEWER", this);
      connect(m_ipcServer, SIGNAL(messageReceived(QString)), this, SLOT(receivedMessage(QString)));
    }
    if (m_ipcClient == nullptr) {
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
  if (m_env != nullptr) { //if I do not abort here application will crash on 'm_res.AsClip()' later
    cerr << qPrintable(tr("Init called on existing environment,..")) << endl;
    return -2;
  }
  bool firstTime = this->minimumSize().width() == 0;
  cout << " first time: " << (firstTime ? "true" : "false") << std::endl;
  bool scrolling = ui.scrollingCheckBox->isChecked();
  bool changeLabelSize = false;
  cout << " scrolling: " << (scrolling ? "true" : "false") << std::endl;
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
    IScriptEnvironment* (*CreateScriptEnvironment)(int version) = (IScriptEnvironment*(*)(int)) avsDLL.resolve("CreateScriptEnvironment"); //resolve CreateScriptEnvironment from the dll
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

    if (scrolling) {
      ui.scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
      ui.scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
      ui.scrollArea->verticalScrollBar()->adjustSize();
      ui.scrollArea->verticalScrollBar()->show();
      ui.scrollArea->horizontalScrollBar()->adjustSize();
      ui.scrollArea->horizontalScrollBar()->show();
    } else {
      ui.scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
      ui.scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
      ui.scrollArea->verticalScrollBar()->resize(0, 0);
      ui.scrollArea->verticalScrollBar()->hide();
      ui.scrollArea->horizontalScrollBar()->resize(0, 0);
      ui.scrollArea->horizontalScrollBar()->hide();
    }
    QString newContent, input = m_currentInput;
    bool invokeFFInfo = false;
    QFile file(input);
    if (file.open(QIODevice::ReadOnly)) {
      bool ffmpegSource = false;
      bool showInfo = false;
      bool mpeg2source = false;
      bool dgnvsource = false;
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
      ui.infoCheckBox->setEnabled(true);
      showInfo = ui.infoCheckBox->isChecked();
      if (showInfo) {
        int index = content.indexOf("distributor()", Qt::CaseInsensitive);
        addShowInfoToContent(index, ffmpegSource, content, newContent, ffms2Line, invokeFFInfo, mpeg2source, dgnvsource);
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
    bool reload = false;
    this->outputColorSpaceInfo();
    if (!m_inf.IsRGB32()) { // make sure color is RGB32
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
    int width = 0, height = 0;
    this->adjustToVideoInfo(scrolling, firstTime, width, height, changeLabelSize);
    cout << qPrintable(" " + tr("Adjusting slider to frame count,..")) << endl;
    ui.frameHorizontalSlider->setMaximum(m_frameCount -1);
    ui.jumpToSpinBox->setMaximum(m_frameCount -1);
    ui.frameHorizontalSlider->resetMarks();
    this->adjustLabelSize(changeLabelSize && (firstTime || ui.histogramCheckBox->isChecked() || !scrolling), width, height); // adjust label size
    this->showFrame(start); //show frame
    this->adjustWindowSize(changeLabelSize, width, height);
  } catch (AvisynthError &err) { //catch AvisynthErrors
    cerr << qPrintable(tr("-> Avisynth error: %1").arg(err.msg)) << endl;
    return -11;
  } catch (...) { //catch everything else
    cerr << qPrintable("->" + tr("Unknown error")) << endl;
    return -12;
  }
  cout << qPrintable(tr("finished initializing the avisynth script environment,..")) << endl;
  if (m_ipcClient != nullptr) {
    m_ipcClient->send_MessageToServer("AvsViewer started ipcClient&Server with id " + m_ipcID);
  }
  if ((!firstTime && !ui.histogramCheckBox->isChecked()) || this->isFullScreen()) {
    return 0;
  }
  if (changeLabelSize && !scrolling) {
    m_showLabel->resize(ui.scrollArea->size());
  }
  return 0;
}

void avsViewer::outputColorSpaceInfo() const
{
  cout << qPrintable(" " + tr("Checking colorspace,..")) << endl;
  if (m_inf.IsRGB32()) {
    cout << qPrintable("  " + tr("current color space is RGB32")) << endl;
  } else if (m_inf.IsRGB()) {
    cout << qPrintable("  " + tr("current color space is RGB")) << endl;
  } else if (m_inf.IsYV12()) {
    cout << qPrintable("  " + tr("current color space is Yv12")) << endl;
  } else if (m_inf.IsRGB24()) {
    cout << qPrintable("  " + tr("current color space is RGB24")) << endl;
  } else if (m_inf.IsYUY2()) {
    cout << qPrintable("  " + tr("current color space is YUY2")) << endl;
  } else if (m_inf.IsYUV()) {
    cout << qPrintable("  " + tr("current color space is YUV")) << endl;
  } else {
    cout << qPrintable("  " + tr("current color space is unknown")) << endl;
  }
}

void avsViewer::adjustToVideoInfo(const bool& scrolling, const bool& first, int& width, int& height, bool& changeLabelSize)
{
  m_frameCount = m_inf.num_frames; //get frame count
  cout << qPrintable(" -> " + tr("Clip contains: %1 frames").arg(m_frameCount)) << endl;
  width = m_inf.width;
  height = m_inf.height;
  if (m_currentFrameWidth != 0) {
    width = m_currentFrameWidth;
    height = m_currentFrameHeight;
  } else {
    m_currentFrameWidth = width;
    m_currentFrameHeight = height;
    changeLabelSize = true;
  }
  if (!scrolling) {
    if (first) {
      while (width > (m_desktopWidth - 50) || height > (m_desktopHeight - 50)) {
        width *= 0.9;
        height *= 0.9;
      }
    } else {
      width = ui.scrollArea->width();
      height = ui.scrollArea->height();
    }
  }
}

/**
 * adjust the size of the label
 **/
void avsViewer::adjustLabelSize(const bool& adjust, const int& width, const int& height)
{
  if (!adjust) {
    return;
  }
  m_showLabel->resize(width, height);
  m_showLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


/**
 * adjust the size of the Window
 **/
void avsViewer::adjustWindowSize(const bool& adjust, const int& width, const int& height)
{
  if (!adjust) {
    return;
  }
  if (width > height) {
    this->resize(this->width(), this->sizeHint().height());
  } else if (width < height){
    this->resize(this->sizeHint().width(), this->height());
  } else {
    this->adjustSize();
  }
}


/**
 * adjusts frame-index and frame to slider position
 **/
void avsViewer::on_frameHorizontalSlider_valueChanged(int value)
{
  if (value < 0) {
    return;
  }
  ui.frameNumberLabel->setText("(" + QString::number(value) + ")"); // update frame label
  if (!ui.frameHorizontalSlider->isSliderDown()) {
    this->showFrame(value); //show current frame
  }
}

void avsViewer::addBordersForFill(int& width)
{
  if (m_fill == 0 || m_noAddBorders) {
    return;
  }
  int add = 16-m_fill;
  try {
    AVSValue args[5] = {m_clip, 0, 0, add, 0};
    m_clip = m_env->Invoke("AddBorders", AVSValue(args, 5)).AsClip();
    m_inf = m_clip->GetVideoInfo();
    width += add;
    cout << " adding borders -> new clip resolution: " << m_inf.width << "x" << m_inf.height << endl;
    m_noAddBorders = true;
  } catch (AvisynthError &err) { //catch AvisynthErrors
    cerr << qPrintable(tr("Avisynth error: ")) << err.msg << endl;
  } catch (...) {
    cerr << "AddBorder failed!" << endl;
  }
}

void avsViewer::cropForFill(QImage& image, int& width, const int& height)
{
  if (m_fill == 0) {
    return;
  }
  cout << " image resolution: " << image.width() << "x" << image.height() << endl;
  width -= (16-m_fill);
  image = image.copy(0, 0, width, height);
  cout << " cropped -> new image resolution: " << image.width() << "x" << image.height() << endl;
}

/**
 * shows frame number i
 **/
void avsViewer::showFrame(const int& i)
{
  if (m_env == nullptr || i > m_frameCount) {
    return;
  }
  cout << " showFrame: " << i << endl;
  cout << "  m_frameCount: " << m_frameCount << endl;
  cout << "  m_env: " << int(m_env != nullptr) << endl;
  try {
    int width = m_inf.width;
    int height = m_inf.height;
    cout << "avisynth frame resolution: " << width << "x" << height << endl;
    cout << "current label resolution: " << m_currentFrameWidth << "x" << m_currentFrameHeight << endl;
    if (m_fill == 0) {
      m_fill = width%16;
    }
    this->addBordersForFill(width);
    PVideoFrame f = m_clip->GetFrame(i, m_env); // get frame number i
    if (f == nullptr) {
      cerr << " couldn't show frame (no frame: " << i << ")" << endl;
      return;
    }

    const unsigned char* data = f->GetReadPtr();
    QImage image(data, width, height, QImage::Format_RGB32); //create a QImage
    this->cropForFill(image, width, height);
    m_showLabel->setText(QString());
    m_currentImage = image.mirrored(); // flip image otherwise it's upside down
    QPixmap map;
    if (!map.convertFromImage(m_currentImage)) {
      cerr << " couldn't convert image data to pixmap,.. (" << i << ")" << endl;
      return;
    }
    m_showLabel->setPixmap(map);
    if (!ui.scrollingCheckBox->isChecked()) {
      m_showLabel->setPixmap(map.scaled(this->width()-8, this->height()-40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else if (m_zoom != 1.0) {
      m_showLabel->setPixmap(map.scaled(int (width*m_zoom+0.5), int (height*m_zoom+0.5), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
      m_showLabel->setPixmap(map);
    }
    m_currentFrameWidth = m_showLabel->width();
    m_currentFrameHeight = m_showLabel->height();
    cout << "-> updated label resolution: " << m_currentFrameWidth << "x" << m_currentFrameHeight << endl;
    m_current = i; //set m_current to i
    ui.frameHorizontalSlider->setSliderPosition(m_current); // adjust the slider position
    QString title = tr("showing frame number: %1 of %2").arg(m_current).arg(m_frameCount); //adjust title bar;
    if (m_dualView) {
      title += " " + tr("(left side = original, right side = filtered; input: %1)").arg(m_currentInput);
    }
    this->setWindowTitle(title);
  } catch (...) {
    cerr << " couldn't show frame,..." << "(" << i << ")" << endl;
  }
}

void avsViewer::killEnv()
{
  cout << "KILL environment" << endl;
  this->cleanUp();
  if (!m_avsModified.isEmpty()) {
    QFile::remove(m_avsModified);
    m_avsModified = QString();
  }
  ui.frameHorizontalSlider->resetMarks();
  cout << "Cleaned environment" << endl;
}

void avsViewer::on_jumpToStartPushButton_clicked()
{
  if (m_frameCount == 0) {
    return;
  }
  this->showFrame(0);
}
void avsViewer::on_jumpToEndPushButton_clicked()
{
  if (m_frameCount == 0) {
    return;
  }
  this->showFrame(m_frameCount -1);
}

void avsViewer::on_jumpToPushButton_clicked()
{
          if (m_frameCount == 0) {
            return;
          }
  int to = ui.jumpToSpinBox->value();
  this->showFrame(to);
}

/**
 * allows to select a .avs file, starts the initialization
 **/
void avsViewer::on_openAvsPushButton_clicked()
{
  m_showLabel->setText(tr("Opening new file,.."));
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
  m_showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  cout << "Current input: " << qPrintable(m_currentInput) << endl;
  this->init();
}

void avsViewer::resizeEvent(QResizeEvent* event)
{
   QWidget::resizeEvent(event);
   if (m_current < 0) {
     m_current = 0;
   }
   this->showFrame(m_current);
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
