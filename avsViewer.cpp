#include "avsViewer.h"
#include <conio.h>
#include <QFile>
#include <QPixmap>
#include <iostream>
#include <QLibrary>
#include <QFileInfo>
#include <QTextCodec>
#include <QMessageBox>
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

const QString SEP1 = " ### ";
const AVS_Linkage *AVS_linkage = nullptr;

avsViewer::avsViewer(QWidget *parent, const QString& path, const double& mult, const QString& ipcID, const QString& matrix)
    : QWidget(parent), ui(), m_frameCount(100), m_current(-1),
        m_currentInput(path), m_version(QString()), m_avsModified(QString()),
        m_inputPath(QString()), m_res(0), m_mult(mult), m_currentImage(),
        m_dualView(false), m_desktopWidth(1920), m_desktopHeight(1080),
        m_ipcID(ipcID), m_currentScriptContent(QString()), m_ipcServer(nullptr), m_ipcClient(nullptr),
        m_matrix(matrix), m_showLabel(new QLabel()), m_zoom(1),
        m_currentFrameWidth(0), m_currentFrameHeight(0), m_fill(0), m_noAddBorders(false), m_env(nullptr),
        m_inf(nullptr), m_providedInput(path)
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
  QScreen *screen = QGuiApplication::primaryScreen();
  QRect  screenGeometry = screen->geometry();
  int height = screenGeometry.height();
  int width = screenGeometry.width();
  m_desktopWidth = width;
  m_desktopHeight = height;
  std::cout << "-> using desktop resolution: " << m_desktopWidth << "x" << m_desktopHeight << std::endl;
  ui.scrollArea->setWidget(m_showLabel);
  if (m_currentInput.isEmpty()) {
    return;
  }
  m_showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  delete ui.openAvsPushButton;
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
  std::cout << qPrintable(tr("Closing,...")) << std::endl;
  if (!m_avsModified.isEmpty()) {
    QFile::remove(m_avsModified);
    std::cout << qPrintable(tr("deleting: %1").arg(m_avsModified)) << std::endl;
  }
  std::cout << qPrintable(tr("finished,...")) << std::endl;
}

bool avsViewer::initEnv()
{
  QLibrary avsDLL("AviSynth.dll");
  if (!avsDLL.load()) { //load avisynth.dll if it's not already loaded and abort if it couldn't be loaded
    QString error = avsDLL.errorString();
    if (!error.isEmpty()) {
      std::cerr << "Could not load avisynth.dll! " << std::endl << qPrintable(error) << std::endl;
      return false;
    }
    std::cerr << "Could not load avisynth.dll!" << std::endl;
    return false;
  }
  std::cout << "loaded avisynth dll,.." << std::endl;
  IScriptEnvironment* (*CreateScriptEnvironment)(int version) = (IScriptEnvironment*(*)(int)) avsDLL.resolve("CreateScriptEnvironment"); //resolve CreateScriptEnvironment from the dll
  std::cout << "loaded CreateScriptEnvironment definition from dll,.." << std::endl;
  m_env = CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION); //create a new IScriptEnvironment
  if (!m_env) { //abort if IScriptEnvironment couldn't be created
    std::cerr << "Could not create IScriptenvironment,..." << std::endl;
    return false;
  }
  return true;
}

bool avsViewer::setRessource()
{
  try {

    AVS_linkage = m_env->GetAVSLinkage();
    const char* infile = m_currentInput.toLocal8Bit(); //convert input name to char*
    std::cout << "Importing " << infile << std::endl;
    m_res = m_env->Invoke("Import", infile); //import current input to environment
    if (!m_res.IsClip()) {
       std::cerr << "Couldn't load input, not a clip!" << std::endl;
       return false;
    }
    if (!m_res.Defined()) {
      QString error = QObject::tr("Couldn't import:") + " " + m_currentInput;
      error += "\r\n";
      error += QObject::tr("Script seems not to be a valid avisynth script.");
      std::cerr << qPrintable(error) << std::endl;
      return false;
    }
    return true;
  } catch (AvisynthError &err) { //catch AvisynthErrors
    std::cerr << "-> " << err.msg << std::endl;
  } catch (...) { //catch everything else
    std::cerr << "-> setRessource: Unknown error" << std::endl;
  }
  return false;
}

bool avsViewer::setVideoInfo()
{
  PClip  clip = m_res.AsClip();    //get clip
  m_inf = &(clip->GetVideoInfo());    //get clip infos
  if (!m_inf->HasVideo()) { //abort if clip has no video
    std::cerr << "Input has no video stream -> aborting" << std::endl;
    return false;
  }
  return true;
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
    this->sendMessageToSever("Clean up old script environment,..");
    m_res = 0;
    try {
      m_env->DeleteScriptEnvironment(); //delete the old script environment
    } catch (AvisynthError &err) { //catch AvisynthErrors
      std::cerr << "Failed to delete script environment " << err.msg << std::endl;
    } catch (...) {
      std::cerr << "Failed to delete script environment,.. (Unkown Error)" << std::endl;
    }
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
  if (!ui.infoCheckBox->isChecked()) {
    std::cout << "resetting input,.. (info)" << std::endl;
    m_currentInput = m_providedInput;
  }
  this->refresh();
}

void avsViewer::on_histogramCheckBox_toggled()
{
  if (!ui.histogramCheckBox->isChecked()) {
    std::cout << "resetting input,.. (histogram)" << std::endl;
    m_currentInput = m_providedInput;
  }
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
  std::cout << "From console reader: " << qPrintable(text) << std::endl;
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
  foreach(QString line, content.split("\n"))
  {
    if (line.contains("MPEG2Source(", Qt::CaseInsensitive)) {
      mpeg2source = true;
      return;
    }
    if (line.contains("DGSource(", Qt::CaseInsensitive)) {
      dgnvsource = true;
      return;
    }
    if (line.contains("FFMpegSource2(", Qt::CaseInsensitive)
        || line.contains("FFVideoSource(", Qt::CaseInsensitive)) {
      ffmpegSource = true;
      continue;
    }
    if (ffmpegSource && !ffms2Line.isEmpty()) {
      return;
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
  }
}

void addShowInfoToContent(const int distributorIndex, const bool ffmpegSource,
    const QString& content, QString &newContent, const QString& ffms2Line,
    bool &invokeFFInfo, const bool mpeg2source, const bool dgnvsource)
{
  if (ffmpegSource) {
    if (distributorIndex != -1) { // contains distributor
      newContent = content;
      if (newContent.contains(QString("FFInfo("))) {
        return;
      }
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
    if (newContent.contains(QString("Info("))) {
      return;
    }
    newContent = newContent.replace(".d2v\"", ".d2v\", info=1", Qt::CaseInsensitive);
  } else if (dgnvsource) {
    if (newContent.contains(QString("Info("))) {
      return;
    }
    newContent = content;
    newContent = newContent.replace(".dgi\"", ".dgi\", show=true", Qt::CaseInsensitive);
  } else if (distributorIndex != -1) { // contains distributor
    if (newContent.contains(QString("Info("))) {
      return;
    }
      newContent = content;
      newContent = newContent.remove(distributorIndex, newContent.size()).trimmed();
      newContent += "\n";
      newContent += "Info()";
      newContent += "\n";
      newContent += "return last";
  } else if (!content.contains(QString("Info("))) {
      newContent = content;
      int index = newContent.lastIndexOf("return");
      if (index != -1) {
        newContent = newContent.remove(index, newContent.size()).trimmed();
      }
      newContent += "Info()";
      newContent += "\n";
      newContent += "return last";
  }
}

void addHistrogramToContent(const QString& content, QString &newContent, const QString& matrix)
{
  if (newContent.isEmpty()) {
    newContent = content;
  }
  int index1 = newContent.indexOf("# adjust color to RGB32");
  int index = newContent.indexOf("StackHorizontal(Source, SourceFiltered)");
  if (newContent.contains(QString("Histogram("))) {
    return;
  }
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

void avsViewer::applyResolution(const QString& content, QString &newContent, double mult, const QString& resize)
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
  if (m_inf->IsFieldBased() && !m_inf->IsRGB()) {
    resizer = resize + "Resize(Ceil(last.Width*" + QString::number(mult) + "), last.Height)\n";
    /*
    Separatefields()
    Shift=(Height()/Float(new_height/2)-1.0)*0.25 # Field shift correction
    even=SelectEven().Spline36Resize(new_width, new_height/2, 0, -Shift, Width(), Height())
    odd=SelectOdd().Spline36Resize(new_width, new_height/2, 0, Shift, Width(), Height())
    Interleave(even,odd)
    Assumefieldbased().AssumeTFF().Weave()
    */
  } else {
    resizer = resize + "Resize(Ceil(last.Width*" + QString::number(mult) + "), last.Height)\n";
  }

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
      std::cout << qPrintable(tr("ignoring received message: %1").arg(message)) << std::endl;
      break;
  }
}

QString avsViewer::getCurrentInput(const QString& script)
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
  QString currentInput = getCurrentInput(m_currentScriptContent); // the input of the avisynth script
  QFile file(value);
  QString newContent;
  sendMessageToSever(QString("reading file,..."));
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
    std::cout << qPrintable(QString("keeping current position: %1").arg(currentPosition)) << std::endl;
  }
  this->killEnv(); // killing old Avisynth environment
  m_currentInput = value; //set current input
  std::cout << "setting provided input,.. (changeTo)";
  m_providedInput = value;
  m_showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  this->init(currentPosition);
}

void avsViewer::callMethod(const QString& typ, const QString& value, const QString &input)
{
  if (!QFile::exists(value)){
    std::cout << qPrintable(QString("Change ignored since '%1' doesn't exist.").arg(value)) << std::endl;
    return;
  }
  this->setWindowTitle(QString("%1, %2:\n%3").arg(typ).arg(value).arg(input));
  if (typ == "changeTo") {
    this->changeTo(input, value);
    return;
  }
  std::cerr << "unsupported typ: " << qPrintable(typ) << std::endl;
  std::cerr << "     with value: " << qPrintable(value) << std::endl;
}

bool avsViewer::adjustScript(bool& invokeFFInfo)
{
  QString newContent;
  QFile file(m_currentInput);
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
    if (m_currentScriptContent.isEmpty()) {
      m_currentScriptContent = content;
    }
    file.close();
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
    std::cerr << qPrintable(tr("Couldn't read content of: %1").arg(m_currentInput)) << std::endl;
    return false;
  }
  if (!newContent.isEmpty()) { //create new modfied avs file
    QString directory = getDirectory(m_currentInput);
    QString name = getFileName(m_currentInput);
    name = name.remove(QString("_tmp"));
    m_avsModified = QDir::toNativeSeparators(directory + QDir::separator() + name + "_tmp.avs");
    if (saveTextTo(newContent, m_avsModified) == 0) {
      m_currentInput = m_avsModified;
      std::cout << "changed content, using: " << qPrintable(m_currentInput) << std::endl;
    }
    std::cout << "changing script content (adjustScript),.." << std::endl;
    m_currentScriptContent = newContent;
  }
  return true;
}

bool avsViewer::invokeFunction(const QString& name)
{
  try {
    const char* function = name.toLocal8Bit();
    m_res = m_env->Invoke(function, AVSValue(&m_res, 1)); //import current input to environment
    std::cout << "invoked " << qPrintable(name) << std::endl;
    return true;
  } catch (AvisynthError &err) { //catch AvisynthErrors
    std::cerr << "Avisynth error " << qPrintable(m_currentInput) << ": " << std::endl << err.msg << std::endl;
  } catch (...) { //catch the rest
    std::cerr << "Unknown C++ exception" << std::endl;
  }
  return false;
}


void avsViewer::initIPC()
{
  if (m_ipcID == QString()) {
    return;
  }
  bool started = false;
  if (m_ipcServer == nullptr) {
    m_ipcServer = new LocalSocketIpcServer(m_ipcID + "AVSVIEWER", this);
    connect(m_ipcServer, SIGNAL(messageReceived(QString)), this, SLOT(receivedMessage(QString)));
    started = true;
  }
  if (m_ipcClient == nullptr) {
    m_ipcClient = new LocalSocketIpcClient(m_ipcID + "HYBRID", this);
    started = true;
  }
  if (started) {
    sendMessageToSever("AvsViewer started ipcClient&Server with id " + m_ipcID);
  }
}

void avsViewer::sendMessageToSever(const QString& message)
{
  std::cout << qPrintable(message) << std::endl;
  this->initIPC();
  if (m_ipcClient != nullptr) {
    m_ipcClient->send_MessageToServer(message);
  }
}

/**
 * initilazing an avisynth environment for the current input file
 **/
int avsViewer::init(int start)
{
  this->initIPC();
  m_current = -1; //reset frameIndex
  if (m_currentInput.isEmpty()) {
    sendMessageToSever(tr("Current input is empty,.."));
    return -1;
  }
  if (m_env != nullptr) { //if I do not abort here application will crash on 'm_res.AsClip()' later
    std::cerr << qPrintable(tr("Init called on existing environment,..")) << std::endl;
    return -2;
  }
  bool firstTime = this->minimumSize().width() == 0;
  sendMessageToSever(tr("Initializing the avisynth script environment,.."));
  if (!this->initEnv()) {
    return -3;
  }
  bool invokeFFInfo = false;
  if (!this->adjustScript(invokeFFInfo)){
    return -4;
  }
  if (!setRessource()) {
    return -5;
  }
  if (!this->setVideoInfo()) {
    return -6;
  }
  this->showVideoInfo();
  bool scrolling = ui.scrollingCheckBox->isChecked();
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
  bool reload = false;
  if (!m_inf->IsRGB32()) { // make sure color is RGB32
    sendMessageToSever(QString("Current color space: %1").arg(this->getColor()));
    if (!this->invokeFunction("ConvertToRGB32")) {
       std::cerr << qPrintable(tr("Couldn't invoke 'ConvertToTGB()' -> aborting")) << std::endl;
      this->killEnv();
      return -9;
    }
    reload = true;
  }
  if (invokeFFInfo) {
    if (!this->invokeFunction("FFInfo")) {
      std::cerr << qPrintable(tr("Couldn't invoke 'FFInfo()' -> aborting")) << std::endl;
      this->killEnv();
      return -10;
    }
    reload = true;
  }
  if (reload) {
    sendMessageToSever(QString(" ") + tr("initializating the clip anew,.."));
    if (!this->setVideoInfo()) { //abort if clip has no video
      std::cerr << qPrintable(tr("Input has no video stream -> aborting")) << std::endl;
      return -11;
    }
  }
  bool changeLabelSize = false;
  int width = 0, height = 0;
  this->adjustToVideoInfo(scrolling, firstTime, width, height, changeLabelSize);
  ui.frameHorizontalSlider->setMaximum(m_frameCount -1);
  ui.jumpToSpinBox->setMaximum(m_frameCount -1);
  ui.frameHorizontalSlider->resetMarks();
  this->adjustLabelSize(changeLabelSize && (firstTime || ui.histogramCheckBox->isChecked() || !scrolling), width, height); // adjust label size
  if (start < 0) {
    start = 0;
  }
  this->showFrame(start); //show frame
  this->adjustWindowSize(changeLabelSize, width, height);
  this->sendMessageToSever(tr("finished initializing the avisynth script environment,.."));

  if ((!firstTime && !ui.histogramCheckBox->isChecked()) || this->isFullScreen()) {
    return 0;
  }
  if (changeLabelSize && !scrolling) {
    m_showLabel->resize(ui.scrollArea->size());
  }
  return 0;
}

/**
 * output the current color space
 *
 * @return String representtion of the current color
 */
QString avsViewer::getColor() const
{
  if (m_inf->IsY8()) {
    return QString("Y8");
  }
  if (m_inf->Is420()) {
    return QString("YV12");
  }
  if (m_inf->IsYUY2()) {
    return QString("YUY2");
  }
  if (m_inf->IsYV16()) {
    return QString("YV16");
  }
  if (m_inf->IsYV24()) {
    return QString("YV24");
  }
  if (m_inf->IsRGB24()) {
    return QString("RGB24");
  }
  if (m_inf->IsRGB32()) {
    return QString("RGB32");
  }
  if (m_inf->IsRGB48()) {
    return QString("RGB48");
  }
  if (m_inf->IsRGB()) {
    return QString("RGB");
  }
  if (m_inf->IsYUV()) {
    return QString("YUV");
  }
  return QString("unknown");
}

/**
 * Show the characteristics of the video
 */
void avsViewer::showVideoInfo()
{
  std::cout << "Color: " << qPrintable(this->getColor());
  std::cout << ", Resolution: " << m_inf->width << "x" << m_inf->height;
  if (m_inf->fps_denominator == 1) {
     std::cout << ", Frame rate: " << m_inf->fps_numerator << " fps";
  } else {
    std::cout << ", Frame rate: " << m_inf->fps_numerator << "/" << m_inf->fps_denominator << " fps";
  }
  m_frameCount = m_inf->num_frames;
  std::cout << ", Length: " << m_frameCount << " frames";
  if (m_inf->IsBFF()) {
    std::cout << ", BFF" << std::endl;
  } else if (m_inf->IsTFF()) {
    std::cout << ", TFF" << std::endl;
  } else {
    std::cout << ", PRO" << std::endl;
  }
  if (m_inf->HasAudio()) {
    int sampleRate = m_inf->audio_samples_per_second;
    if (sampleRate != 0) {
      std::cout << "Audio:" << std::endl;
      std::cout << "Sample rate: " << sampleRate << " Hz";
      std::cout << ", Channel count: " << m_inf->nchannels << std::endl;
    }
  }
  std::cout << std::endl;
}

void avsViewer::adjustToVideoInfo(const bool& scrolling, const bool& first, int& width, int& height, bool& changeLabelSize)
{
  this->showVideoInfo();
  width = m_inf->width;
  height = m_inf->height;
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
    AVSValue args[5] = {m_res.AsClip(), 0, 0, add, 0};
    m_res = m_env->Invoke("AddBorders", AVSValue(args, 5)).AsClip();
    if (!this->setVideoInfo()) {
      return;
    }
    width += add;
    m_noAddBorders = true;
  } catch (AvisynthError &err) { //catch AvisynthErrors
    std::cerr << qPrintable(tr("Avisynth error: ")) << err.msg << std::endl;
  } catch (...) {
    std::cerr << "AddBorder failed!" << std::endl;
  }
}

void avsViewer::cropForFill(QImage& image, int& width, const int& height)
{
  if (m_fill == 0) {
    return;
  }
  width -= (16-m_fill);
  image = image.copy(0, 0, width, height);
  sendMessageToSever(QString(" cropped -> new image resolution: %1x%2").arg(image.width()).arg(image.height()));
}

void avsViewer::outputResType()
{
 if (m_res.IsBool()) {
   std::cerr << "Res is bool" << std::endl;
 }
 if (m_res.IsClip()) {
   std::cerr << "Res is clip" << std::endl;
 }
 if (m_res.IsArray()) {
   std::cerr << "Res is array" << std::endl;
 }
 if (m_res.IsFloat()) {
   std::cerr << "Res is float" << std::endl;
 }
 if (m_res.IsString()) {
   std::cerr << "Res is string" << std::endl;
 }
}

unsigned char* avsViewer::getFrameData(const int& i, const int& count)
{
  try {
    PClip clip = m_res.AsClip();    //get clip
    PVideoFrame pvframe = clip->GetFrame(i, m_env); // get frame number i
    if (pvframe == nullptr) {
      std::cerr << " couldn't show frame (no frame: " << i << ")" << std::endl;
      return nullptr;
    }
    return const_cast<unsigned char*>(pvframe->GetReadPtr());
  } catch (AvisynthError &err) { //catch AvisynthErrors
    std::cerr << "-> " << err.msg << std::endl;
  } catch (...) { //catch everything else
    std::cout << "-> getFrameData - Unknown error (" << count << ")" << std::endl;
  }
  return nullptr;
}

/**
 * shows frame number i
 **/
void avsViewer::showFrame(const int& i)
{
  if (m_env == nullptr || i > m_frameCount) {
    return;
  }
  try {
    int width = m_inf->width;
    int height = m_inf->height;
    if (m_fill == 0) {
      m_fill = width%16;
    }
    this->addBordersForFill(width);
    int count = 0;
    unsigned char* data = this->getFrameData(i, count);
    while (data == nullptr && count < 10) {
      data = this->getFrameData(i, count);
      count++;
    }
    if (data == nullptr) {
      std::cerr << " could not get PVideoFrame data (" << i << ")" << std::endl;
      return;
    }
    QImage image(data, width, height, QImage::Format_RGB32); //create a QImage
    this->cropForFill(image, width, height);
    m_showLabel->setText(QString());
    m_currentImage = image.mirrored(); // flip image otherwise it's upside down
    QPixmap map;
    if (!map.convertFromImage(m_currentImage)) {
      std::cerr << " couldn't convert image data to pixmap,.. (" << i << ")" << std::endl;
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
    m_current = i; //set m_current to i
    ui.frameHorizontalSlider->setSliderPosition(m_current); // adjust the slider position
    QString title = tr("showing frame number: %1 of %2").arg(m_current).arg(m_frameCount); //adjust title bar;
    if (m_dualView) {
      if (m_currentScriptContent.contains(QString("Interleave(Source, SourceFiltered)"))) {
        title += " " + tr("(interleaved, input: %1)").arg(m_currentInput);
      } else if (m_currentScriptContent.contains(QString("Source, SourceFiltered"))) {
        title += " " + tr("(left side = original, right side = filtered; input: %1)").arg(m_currentInput);
      } else if (m_currentScriptContent.contains(QString("SourceFiltered, Source"))) {
        title += " " + tr("(left side = filtered, right side = original; input: %1)").arg(m_currentInput);
      } else {
        title += " " + tr("(input: %1)").arg(m_currentInput);
      }
    }
    this->setWindowTitle(title);
  } catch (...) {
    std::cerr << " couldn't show frame,..." << "(" << i << ")" << std::endl;
  }
}

void avsViewer::killEnv()
{
   qApp->processEvents();
  sendMessageToSever(QString("KILL environment"));
  this->cleanUp();
  if (!m_avsModified.isEmpty()) {
    QFile::remove(m_avsModified);
    m_avsModified = QString();
  }
  ui.frameHorizontalSlider->resetMarks();
  sendMessageToSever("Cleaned environment");
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
    std::cerr << "Current input is empty or not an .avs file,.." << std::endl;
    return;
  }
  m_currentScriptContent = QString();
  this->killEnv();

  m_currentInput = input; //set current input
  m_showLabel->setText(tr("Preparing environment for %1").arg(m_currentInput));
  this->init();
}

/**
 * refresh frame on resize event
 */
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
