#include "avsViewer.h"
#ifdef _WIN32
    #include <conio.h>
    #define AVISYNTH_LIB "AviSynth.dll"
#else
    #ifdef __APPLE__
        #define AVISYNTH_LIB "libavisynth.dylib"
    #else
        #define AVISYNTH_LIB "libavisynth.so"
    #endif
#endif
#include <QFile>
#include <QPixmap>
#include <iostream>
#include <QLibrary>
#include <QFileInfo>
//#include <QTextCodec>
#include <QMessageBox>
#include <QFileDialog>
#include <QStringList>
#include <QTextStream>
#include <QApplication>
#include <QListWidgetItem>
#include "LocalSocketIpcServer.h"
#include "LocalSocketIpcClient.h"
#include <QThread>
#include <QEvent>
#include <QWheelEvent>
#include <QFile>
#include <QScrollBar>
#include <QScreen>
#include <QTimer>
#include <QGridLayout>

const QString SEP1 = " ### ";
const AVS_Linkage *AVS_linkage = 0;

avsViewer::avsViewer(QWidget *parent, const QString& path, const double& mult, const QString& ipcID, const QString& matrix)
    : QWidget(parent), ui(), m_frameCount(100), m_current(-1),
        m_currentInput(path), m_avsModified(QString()), m_lastError(QString()),
        m_inputPath(QString()), m_res(0), m_mult(mult), m_currentImage(),
        m_dualView(false), m_desktopWidth(1920), m_desktopHeight(1080),
        m_ipcID(ipcID), m_currentScriptContent(QString()), m_ipcServer(nullptr), m_ipcClient(nullptr),
        m_ipcClientThread(nullptr),
        m_matrix(matrix), m_showLabel(new QLabel()), m_zoom(1),
        m_snapTimer(new QTimer(this)), m_adjusting(false), m_firstInit(true), m_env(nullptr),
        m_inf(nullptr), m_providedInput(path), m_avsDLL(this), m_showOnly(false)
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
  m_showLabel->setAlignment(Qt::AlignCenter); // whatever slack is left over, split it evenly
  m_showLabel->installEventFilter(this); // see eventFilter(): click the image, focus the slider
  ui.scrollArea->viewport()->installEventFilter(this);
  m_snapTimer->setSingleShot(true);
  m_snapTimer->setInterval(120); // long enough to coalesce a window drag into one adjustment
  connect(m_snapTimer, &QTimer::timeout, this, &avsViewer::snapWindowToAspect);
  QString avisynthDll = QDir::toNativeSeparators(qApp->applicationDirPath() + QDir::separator() + QString(AVISYNTH_LIB));
  if (!QFile::exists(avisynthDll)) {
    avisynthDll = QString(AVISYNTH_LIB);
  }
  m_avsDLL.setFileName(avisynthDll);
  if (!m_currentInput.isEmpty()) {
    delete ui.openAvsPushButton;
    delete ui.histogramCheckBox;
    m_showOnly = true;
  }
  // Run the layout now, after the show-only deletions so it reflects the controls that remain:
  // init() below needs the scroll area to have a real size, and until the layout has been
  // activated once it still carries the default 100x30 it was constructed with.
  if (this->layout() != nullptr) {
    this->layout()->activate();
  }
  if (m_showOnly) {
    this->reportInitError(this->init(0));
  }
}

// show a failed init() in the window instead of only on stderr
void avsViewer::reportInitError(int code)
{
  if (code == 0) {
    return;
  }
  QString reason;
  switch (code) {
    case -1  : reason = tr("No input script was given."); break;
    case -2  : reason = tr("A script environment is still open."); break;
    case -3  : reason = tr("Could not load Avisynth or create a script environment."); break;
    case -4  : reason = tr("Could not read the script."); break;
    case -5  : reason = tr("Avisynth could not import the script."); break;
    case -6  :
    case -11 : reason = tr("The script has no video stream."); break;
    case -9  : reason = tr("Could not convert the clip to RGB32."); break;
    case -10 : reason = tr("Could not invoke FFInfo()."); break;
    default  : reason = tr("Unknown error (%1).").arg(code); break;
  }
  QString text = tr("Could not load:") + "\n" + m_providedInput + "\n\n" + reason;
  if (!m_lastError.isEmpty()) {
    text += "\n\n" + m_lastError;
  }
  m_showLabel->setPixmap(QPixmap());
  m_showLabel->setWordWrap(true);
  m_showLabel->setText(text);
  std::cerr << qPrintable(text) << std::endl;
}
// clicking the image focuses the frame slider, so left/right step frames straight away
bool avsViewer::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() == QEvent::MouseButtonPress
      && (watched == m_showLabel || watched == ui.scrollArea->viewport())
      && ui.zoomHandlingComboBox->currentText() != "Fixed zoom") { // there the drag pans instead
    ui.frameHorizontalSlider->setFocus(Qt::MouseFocusReason);
  }
  return QWidget::eventFilter(watched, event); // never consume it, panning still needs the press
}

void avsViewer::wheelEvent(QWheelEvent* event)
{
  double movedby = event->angleDelta().y();
  if (movedby != 0.0) {
    // Prüfen, ob STRG gedrückt wird
    if (event->modifiers() & Qt::ControlModifier) {
      // Wert der zoomScaleDoubleSpinBox anpassen
      double newZoom = ui.zoomScaleDoubleSpinBox->value();
      newZoom = newZoom + movedby * 0.001;
      newZoom = qMax(1.0, newZoom);
      newZoom = qMin(100.0, newZoom);
      ui.zoomScaleDoubleSpinBox->setValue(newZoom);
    }
    else {
      // Slider-Position anpassen
      int numSteps = int(movedby / 120 + m_current);
      if (numSteps < 0) {
        numSteps = 0;
      }
      if (numSteps >= m_frameCount) {
        numSteps = m_frameCount - 1;
      }
      ui.frameHorizontalSlider->setSliderPosition(numSteps);
    }
    event->accept();
  }
}

avsViewer::~avsViewer()
{
  std::cout << qPrintable(tr("Closing,...")) << std::endl;
  // stop the client thread first: it deletes the client, so m_ipcClient dangles after this
  if (m_ipcClientThread != nullptr) {
    m_ipcClientThread->quit();
    if (!m_ipcClientThread->wait(5000)) {
      m_ipcClientThread->terminate();
      m_ipcClientThread->wait(1000);
    }
    m_ipcClient = nullptr;
  }
  // release the environment instead of leaking it; inline, since killEnv() would send IPC during destruction
  if (m_env != nullptr) {
    m_res = 0;
    try {
      m_env->DeleteScriptEnvironment();
    } catch (AvisynthError err) {
      std::cerr << "Failed to delete script environment on close: " << err.msg << std::endl;
    } catch (...) {
      std::cerr << "Failed to delete script environment on close,.. (Unknown Error)" << std::endl;
    }
    m_env = nullptr;
    m_inf = nullptr;
  }
  if (!m_avsModified.isEmpty()) {
    std::cout << qPrintable(tr("deleting: %1").arg(m_avsModified)) << std::endl;
    QFile::remove(m_avsModified);
  }
  std::cout << qPrintable(tr("finished,...")) << std::endl;
}

bool avsViewer::loadAvisynthDLL()
{
  if (m_avsDLL.isLoaded()) {
    return true;
  }
  if (!m_avsDLL.load()) {
    QString error = m_avsDLL.errorString();
    if (!error.isEmpty()) {
      std::cerr << "Could not load " AVISYNTH_LIB "! " << std::endl << qPrintable(error) << std::endl;
      return false;
    }
    std::cerr << "Could not load " AVISYNTH_LIB "!" << std::endl;
    return false;
  }
  std::cout << "loaded avisynth dll,..(" << qPrintable(m_avsDLL.fileName()) << ")" << std::endl;
  return true;
}

bool avsViewer::initEnv()
{
  //load avisynth.dll if it's not already loaded and abort if it couldn't be loaded
  if (!this->loadAvisynthDLL()) {
    return false;
  }
  // delete script environment in case it exists
  if (m_env != nullptr && m_env != 0) {
    std::cout << "clear script environment" << std::endl;
    m_env->DeleteScriptEnvironment();
    m_env = 0;
  }
  // create new script environment
  IScriptEnvironment* (*CreateScriptEnvironment)(int version) = (IScriptEnvironment*(*)(int)) m_avsDLL.resolve("CreateScriptEnvironment"); //resolve CreateScriptEnvironment from the dll
  if (CreateScriptEnvironment == nullptr) { // resolve() returns null when the symbol is missing
    std::cerr << "Could not resolve 'CreateScriptEnvironment' from " << qPrintable(m_avsDLL.fileName())
              << " - wrong or damaged Avisynth library?" << std::endl;
    return false;
  }
  std::cout << "loaded CreateScriptEnvironment definition from dll,.. " << std::endl;
  m_env = CreateScriptEnvironment(AVISYNTH_INTERFACE_VERSION); //create a new IScriptEnvironment
  if (!m_env) { //abort if IScriptEnvironment couldn't be created
    m_env = CreateScriptEnvironment(AVISYNTH_CLASSIC_INTERFACE_VERSION); //create a new IScriptEnvironment
    if (!m_env) { //abort if IScriptEnvironment couldn't be created
      std::cerr << "Could not create IScriptenvironment,..." << std::endl;
      return false;
    } else {
      std::cout << "Created script env (classic): " << AVISYNTH_CLASSIC_INTERFACE_VERSION << std::endl;
    }
  } else {
     std::cout << "Created script env (current): " << AVISYNTH_INTERFACE_VERSION << std::endl;
  }

  return true;
}

bool avsViewer::setRessource()
{
  m_lastError.clear();
  try {
    QByteArray ba = m_currentInput.toLocal8Bit();
    const char *infile = ba.data();
    std::cerr << "Importing " << infile << std::endl;
    AVS_linkage = m_env->GetAVSLinkage();
    AVSValue filename = infile;
    AVSValue args = AVSValue(&filename, 1);
    m_res = m_env->Invoke("Import", args, 0);
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
  } catch (AvisynthError err) { //catch AvisynthErrors
    m_lastError = QString::fromLocal8Bit(err.msg);
    std::cerr << "-> " << err.msg << std::endl;
  } catch( const std::exception & ex ) {
    std::cout << ex.what() << std::endl;
  }catch (...) { //catch everything else
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

static QString removeLastSeparatorFromPath(QString input)
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

static QString getDirectory(const QString& input)
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
  if (!m_currentImage.save(input, "PNG", -1)) { // -1 = default compression; 100 would mean *least* compressed
    QMessageBox::warning(nullptr, "Error", QObject::tr("Couldn't save %1").arg(input));
  }
  this->showFrame(m_current);
}

void avsViewer::cleanUp()
{
  if (m_env != 0) {
    this->sendMessageToSever("Clean up old script environment,..");
    m_res = 0;
    try {
      m_env->DeleteScriptEnvironment(); //delete the old script environment
    } catch (AvisynthError err) { //catch AvisynthErrors
      std::cerr << "Failed to delete script environment " << err.msg << std::endl;
    } catch (...) {
      std::cerr << "Failed to delete script environment,.. (Unkown Error)" << std::endl;
    }
    m_env = nullptr; // ensure new environment created next time
    m_inf = nullptr; // borrowed from the clip we just released, don't keep it around
    m_current = 0;
    qApp->processEvents();
  }
}

void avsViewer::refresh()
{
  int curr = m_current;
  this->cleanUp();
  m_current = curr;
  // always rewrite from the provided script; re-reading our own _tmp.avs appended the resizer twice
  m_currentInput = m_providedInput;
  this->reportInitError(this->init(m_current));
}

void avsViewer::on_infoCheckBox_toggled()
{
  this->refresh(); // refresh() resets the input to m_providedInput
}

void avsViewer::on_histogramCheckBox_toggled()
{
  if (m_showOnly) {
    return;
  }
  this->refresh(); // refresh() resets the input to m_providedInput
}
void avsViewer::on_zoomScaleDoubleSpinBox_valueChanged(const double& value)
{
  Q_UNUSED(value)
  if (ui.zoomHandlingComboBox->currentText() == "Fixed zoom") {
    this->updatePixmap(); // zooming only re-scales, no need to decode again
  }
}

void avsViewer::on_zoomHandlingComboBox_currentTextChanged(const QString& value)
{
  ui.zoomScaleDoubleSpinBox->setEnabled(value == "Fixed zoom");
  this->updatePixmap();
}

void avsViewer::on_aspectRatioAdjustmentComboBox_currentTextChanged(const QString& value)
{
  Q_UNUSED(value);
  this->refresh();
}

static QString removeQuotes(QString input)
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

static QString getWholeFileName(const QString& input)
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

static QString getFileName(const QString& input)
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

static int saveTextTo(const QString& text, const QString& to)
{
  if (text.isEmpty()) {
    return -1;
  }
  QFile file(to);
  file.remove();
  // no QIODevice::Text: on Windows it re-translated the CR the content already had, one more per rewrite
  if (file.open(QIODevice::WriteOnly)) {
    QTextStream out(&file);
#if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
  out.setEncoding(QStringConverter::System);
#else
  out.setCodec("System");
#endif
    out << text;
    if (file.exists()) {
      file.close();
      return 0;
    }
  }
  return -1;
}

static void checkInputType(const QString& content, bool &ffmpegSource, bool &mpeg2source, bool &dgnvsource, QString &ffms2Line)
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

// detectable = content minus comments, used for the "already present?" tests; rewriting still uses content
static void addShowInfoToContent(const bool ffmpegSource,
    const QString& content, const QString& detectable, QString &newContent, const QString& ffms2Line,
    bool &invokeFFInfo, const bool mpeg2source, const bool dgnvsource)
{
  const QString prefetch = QString("PreFetch(");
  if (ffmpegSource) {
    if (detectable.contains(QString("FFInfo("))) {
      return;
    }
    if (!ffms2Line.isEmpty()) {
      newContent = content;
      int index = content.lastIndexOf(prefetch);
      if (index != -1) {
         QString addition = "\n";
         addition += "Import(\"" + ffms2Line + "\")";
         addition += "\n";
         addition += "FFInfo()";
         addition += "\n";
         newContent = newContent.insert(index, addition);
         return;
      }
      index = newContent.lastIndexOf("return ");
      if (index != -1) {
        newContent = newContent.remove(index, newContent.size()).trimmed();
      }
      // a dead "SetModeMT("/"SeMTMode(5)" block was dropped here, not respelled: Avisynth+ has no SetMTMode
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
    if (detectable.contains(QString("Info("))) {
      return;
    }
    newContent = content;
    newContent = newContent.replace(".d2v\"", ".d2v\", info=1", Qt::CaseInsensitive);
  } else if (dgnvsource) {
    if (detectable.contains(QString("Info("))) {
      return;
    }
    newContent = content;
    newContent = newContent.replace(".dgi\"", ".dgi\", show=true", Qt::CaseInsensitive);
  } else if (!detectable.contains(QString("Info("))) {
    newContent = content;
    int index = newContent.lastIndexOf(prefetch);
    if (index != -1 && !ffms2Line.isEmpty()) {
       QString addition = "\n";
       addition += "Import(\"" + ffms2Line + "\")";
       addition += "\n";
       addition += "FFInfo()";
       addition += "\n";
       newContent = newContent.insert(index, addition);
       return;
    }
    index = newContent.lastIndexOf("return ");
    if (index != -1) {
      newContent = newContent.remove(index, newContent.size()).trimmed();
    }
    newContent += "\n";
    newContent += "Info()";
    newContent += "\n";
    newContent += "return last";
  }
}

static void addHistrogramToContent(const QString& content, QString &newContent, const QString& matrix)
{
  if (newContent.isEmpty()) {
    newContent = content;
  }
  int index1 = newContent.lastIndexOf("ConvertToRGB32(");
  if (index1 == -1){
    index1 = newContent.lastIndexOf("PreFetch(");
  }
  if (index1 == -1){
    index1 = newContent.lastIndexOf("return ");
  }
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
  // let Avisynth decide mod2 at evaluation time; m_inf is not valid yet this early in init()
  const QString width = QString("Ceil(last.Width*%1)").arg(mult);
  const QString target = QString("last.IsRGB32() ? %1 : %1 - (%1 % 2)").arg(width);
  QString resizer = resize + QString("Resize(%1, last.Height)\n").arg(target);
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
  this->setWindowTitle(message + this->matrixSuffix());
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
  const int end = input.indexOf("\n");
  if (end != -1) { // QString::remove() counts a negative position back from the
    input = input.remove(end, input.size()); // end, so -1 silently ate a character
  }
  return input.trimmed(); // drop the '\r' of a CRLF script, and any stray padding
}

void avsViewer::changeTo(const QString& input, const QString& value)
{
  int currentPosition = 0;
  bool scrolling = ui.zoomHandlingComboBox->currentText() != "Fit to frame";
  int hPos, vPos;
  bool hVisible = false, vVisible = false;
  int hMax = 0, vMax = 0;
  if (scrolling) {
    hPos = ui.scrollArea->horizontalScrollBar()->sliderPosition();
    hVisible = ui.scrollArea->horizontalScrollBar()->isVisible();
    hMax = ui.scrollArea->horizontalScrollBar()->maximum();
    vPos = ui.scrollArea->verticalScrollBar()->sliderPosition();
    vVisible = ui.scrollArea->verticalScrollBar()->isVisible();
    vMax = ui.scrollArea->verticalScrollBar()->maximum();
  }

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
  this->reportInitError(this->init(currentPosition));
  if (scrolling) {
    ui.scrollArea->horizontalScrollBar()->setVisible(hVisible);
    if (hVisible) {
      ui.scrollArea->horizontalScrollBar()->setMaximum(hMax);
      ui.scrollArea->horizontalScrollBar()->adjustSize();
      ui.scrollArea->horizontalScrollBar()->setSliderPosition(hPos);
    }
    ui.scrollArea->verticalScrollBar()->setVisible(vVisible);
    if (vVisible) {
      ui.scrollArea->verticalScrollBar()->setMaximum(vMax);
      ui.scrollArea->verticalScrollBar()->adjustSize();
      ui.scrollArea->verticalScrollBar()->setSliderPosition(vPos);
    }
    this->resize(this->size().width()+2, this->size().height()+2);
    this->resize(this->size().width()-2, this->size().height()-2);
  }
}

void avsViewer::callMethod(const QString& typ, const QString& value, const QString &input)
{
  // an .avs may LoadPlugin arbitrary code, so hold IPC commands to the same rule as the Open button
  if (!value.endsWith(".avs", Qt::CaseInsensitive)) {
    std::cerr << qPrintable(QString("Change ignored, not an .avs file: '%1'").arg(value)) << std::endl;
    return;
  }
  if (!QFile::exists(value)){
    std::cout << qPrintable(QString("Change ignored since '%1' doesn't exist.").arg(value)) << std::endl;
    return;
  }
  this->setWindowTitle(QString("%1, %2: %3").arg(typ).arg(value).arg(input) + this->matrixSuffix());
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
    // detect on the comment-stripped script; this list used to be built and then thrown away
    QStringList nocomments;
    foreach(QString line, lines) {
      line = line.trimmed();
      if (line.isEmpty() || line.startsWith('#')) {
        continue;
      }
      nocomments << line;
    }
    const QString detectable = nocomments.join("\n");
    if (m_currentScriptContent.isEmpty()) {
      m_currentScriptContent = content;
    }
    file.close();
    m_dualView = detectable.contains("SourceFiltered = Source");
    checkInputType(detectable, ffmpegSource, mpeg2source, dgnvsource, ffms2Line);
    ui.infoCheckBox->setEnabled(true);
    showInfo = ui.infoCheckBox->isChecked();
    if (showInfo) {
      addShowInfoToContent(ffmpegSource, content, detectable, newContent, ffms2Line, invokeFFInfo, mpeg2source, dgnvsource);
    }
    if (!m_showOnly && ui.histogramCheckBox->isChecked()) {
      addHistrogramToContent(content, newContent, m_matrix);
    }
    applyResolution(content, newContent, m_mult, ui.aspectRatioAdjustmentComboBox->currentText());
  }
  else {
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
    QByteArray ba = name.toLocal8Bit();
    const char *function = ba.data();
    std::cout << "invoking " << function << std::endl;
    if (!m_env->FunctionExists(function)) {
       m_env->ThrowError(ba + " does not exist!");
    }
    m_res = m_env->Invoke(function, AVSValue(&m_res, 1)); //import current input to environment
    return true;
  } catch (AvisynthError err) { //catch AvisynthErrors
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
    // own thread: connectToServer() blocks for seconds on a busy pipe. No parent - moveToThread() refuses one
    m_ipcClientThread = new QThread(this);
    m_ipcClientThread->setObjectName("ipc-client");
    m_ipcClient = new LocalSocketIpcClient(m_ipcID + "HYBRID", nullptr);
    m_ipcClient->moveToThread(m_ipcClientThread);
    // destroy the client on its own thread, where its socket lives
    connect(m_ipcClientThread, &QThread::finished, m_ipcClient, &QObject::deleteLater);
    m_ipcClientThread->start();
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
    // queued, never direct: the client is on another thread and may be blocked in connect
    QMetaObject::invokeMethod(m_ipcClient, "send_MessageToServer", Qt::QueuedConnection,
                              Q_ARG(QString, message));
  }
}

// initilazing an avisynth environment for the current input file
int avsViewer::init(int start)
{
  this->initIPC();
  m_current = -1; //reset frameIndex
  if (m_currentInput.isEmpty()) {
    sendMessageToSever(tr("Current input is empty,.."));
    return -1;
  }
  if (m_env != nullptr && m_env != 0) { //if I do not abort here application will crash on 'm_res.AsClip()' later
    std::cerr << qPrintable(tr("Init called on existing environment,..")) << std::endl;
    return -2;
  }
  // m_firstInit, not 'minimumSize().width() == 0': the constructor now activates the layout
  // before calling init(0), and activating a layout is what gives a window its minimum size
  const bool firstTime = m_firstInit;
  sendMessageToSever(tr("Initializing the avisynth script environment,.."));
  if (!this->initEnv()) {
    return -3;
  }
  bool invokeFFInfo = false;
  if (!this->adjustScript(invokeFFInfo)){
    return -4;
  }
  if (!this->setRessource()) {
    return -5;
  }
  if (!this->setVideoInfo()) {
    return -6;
  }
  this->showVideoInfo();
  QString zoom = ui.zoomHandlingComboBox->currentText();
  bool scrolling = zoom != "Fit to frame";
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
    ui.scrollArea->verticalScrollBar()->hide();
    ui.scrollArea->horizontalScrollBar()->hide();
  }
  bool reload = false;
  if (!m_inf->IsRGB32()) { // make sure color is RGB32
    sendMessageToSever(QString("Current color space: %1").arg(this->getColor()));
    if (!this->invokeFunction("ConvertToRGB32")) {
       std::cerr << qPrintable(tr("Couldn't invoke 'ConvertToRGB32()' -> aborting")) << std::endl;
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
    this->showVideoInfo(); // the clip changed under us, so re-log it and re-read m_frameCount
  }
  bool changeLabelSize = false;
  int width = 0, height = 0;
  this->adjustToVideoInfo(scrolling, firstTime, width, height, changeLabelSize);
  ui.frameHorizontalSlider->setMaximum(m_frameCount -1);
  ui.jumpToSpinBox->setMaximum(m_frameCount -1);
  ui.frameHorizontalSlider->resetMarks();
  // size the window before decoding, so the frame is scaled once, straight to its final size
  this->adjustWindowSize(changeLabelSize, width, height);
  this->adjustLabelSize(changeLabelSize, width, height);
  if (start < 0) {
    start = 0;
  }
  this->showFrame(start); //show frame
  this->sendMessageToSever(tr("finished initializing the avisynth script environment,.."));
  m_firstInit = false;
  // Info, Histogram or a different script can all change the clip's aspect ratio, so let the
  // window follow it. Does nothing when the window is already at the right ratio.
  m_snapTimer->start();
  return 0;
}

// string representation of the current color space
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

// Show the characteristics of the video
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

// The area the frame is actually drawn into. This deliberately reads the scroll area and
// not its viewport: the viewport only gets a real size after show(), which is too late for
// the init(0) the constructor runs, while the scroll area is correct as soon as the layout
// has been activated. See docs/fit-to-frame-plan.md for the measurements behind that.
QSize avsViewer::frameAreaSize() const
{
  QSize area = ui.scrollArea->size();
  if (ui.scrollArea->verticalScrollBar()->isVisible()) {
    area.rwidth() -= ui.scrollArea->verticalScrollBar()->width();
  }
  if (ui.scrollArea->horizontalScrollBar()->isVisible()) {
    area.rheight() -= ui.scrollArea->horizontalScrollBar()->height();
  }
  if (area.width() > 32 && area.height() > 32) {
    return area;
  }
  // The layout has not run yet, so the scroll area still has its default size. Everything
  // around it does have a usable size hint, so add those up rather than guess a constant.
  const QGridLayout* grid = qobject_cast<const QGridLayout*>(this->layout());
  if (grid == nullptr) {
    return this->size();
  }
  const QMargins margins = grid->contentsMargins();
  const int spacing = qMax(0, grid->verticalSpacing());
  const int chromeHeight = margins.top() + margins.bottom() + 3 * spacing
                         + ui.frameHorizontalSlider->sizeHint().height()
                         + ui.horizontalLayout->sizeHint().height()
                         + ui.horizontalLayout_2->sizeHint().height();
  return QSize(this->width() - margins.left() - margins.right(),
               this->height() - chromeHeight);
}

// Keep the frame area at the clip's aspect ratio, so the image fills it exactly instead of
// leaving empty space beside or above it. Maximised and full screen are exempt: there the
// window size is not ours to choose, so the image is centred and letterboxed instead.
void avsViewer::snapWindowToAspect()
{
  if (m_adjusting || !this->isVisible() || m_currentImage.isNull()) {
    return;
  }
  if (this->isMaximized() || this->isFullScreen()) {
    return;
  }
  // 'Fixed zoom' scales by hand and scrolls, so shrink-wrapping the window would fight the
  // user. The other two modes fit the image to the frame area, which is what this is for.
  if (ui.zoomHandlingComboBox->currentText() == "Fixed zoom") {
    return;
  }
  const int imageWidth = m_currentImage.width();
  const int imageHeight = m_currentImage.height();
  if (imageWidth <= 0 || imageHeight <= 0) {
    return;
  }
  const QScreen* screen = this->screen();
  if (screen == nullptr) {
    return;
  }
  const QSize chrome = this->size() - this->frameAreaSize();
  // resize() sets the client area; the window manager puts its frame around that
  const QRect available = screen->availableGeometry();
  const QRect frameBefore = this->frameGeometry();
  const int decorationWidth = qMax(0, frameBefore.width() - this->width());
  const int decorationHeight = qMax(0, frameBefore.height() - this->height());
  const int maxWidth = available.width() - decorationWidth;
  const int maxHeight = available.height() - decorationHeight;
  int wantedWidth = this->width();
  int wantedHeight = qRound((wantedWidth - chrome.width()) * imageHeight / double(imageWidth))
                     + chrome.height();
  if (wantedHeight > maxHeight) { // would grow off screen, so take the height and follow with the width
    wantedHeight = maxHeight;
    wantedWidth = qMin(maxWidth,
                       qRound((wantedHeight - chrome.height()) * imageWidth / double(imageHeight))
                       + chrome.width());
  }
  if (wantedWidth == this->width() && wantedHeight == this->height()) {
    return;
  }
  m_adjusting = true; // resize() re-enters resizeEvent(), which must not schedule another snap
  this->resize(wantedWidth, wantedHeight);
  // Growing only ever adds to the bottom and the right, so a window sitting low on the screen
  // would push its control rows off the edge. Pull it back in rather than leave them unreachable.
  // frameGeometry() still reports the old rectangle here, so work out where the window lands
  // from the size we just asked for and the decorations measured before the resize.
  const QRect frame(frameBefore.topLeft(),
                    QSize(wantedWidth + decorationWidth, wantedHeight + decorationHeight));
  QPoint shift(qMin(0, available.right() - frame.right()), qMin(0, available.bottom() - frame.bottom()));
  shift.rx() += qMax(0, available.left() - (frame.left() + shift.x()));
  shift.ry() += qMax(0, available.top() - (frame.top() + shift.y()));
  if (!shift.isNull()) {
    this->move(this->pos() + shift);
  }
  m_adjusting = false;
}

// works out how large the frame area should be for the current clip
void avsViewer::adjustToVideoInfo(const bool& scrolling, const bool& first, int& width, int& height, bool& changeLabelSize)
{
  width = m_inf->width;
  height = m_inf->height;
  if (first) { // only the first init sizes the window, later ones keep what the user set
    changeLabelSize = true;
    while (width > (m_desktopWidth - 50) || height > (m_desktopHeight - 50)) {
      width = qRound(width * 0.9);
      height = qRound(height * 0.9);
    }
    return;
  }
  if (!scrolling) { // fitting: the label covers the frame area, whatever size the clip is
    const QSize area = this->frameAreaSize();
    width = area.width();
    height = area.height();
  }
}

// adjust the size of the label
void avsViewer::adjustLabelSize(const bool& adjust, const int& width, const int& height)
{
  if (!adjust) {
    return;
  }
  m_showLabel->resize(width, height);
  m_showLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


// Size the window so that its frame area comes out exactly width x height. Driving this from
// the clip size rather than from sizeHint() is what keeps it from collapsing onto the control
// rows: sizeHint() follows the label, which follows a pixmap that has not been scaled yet.
void avsViewer::adjustWindowSize(const bool& adjust, const int& width, const int& height)
{
  if (!adjust) {
    return;
  }
  const QSize chrome = this->size() - this->frameAreaSize();
  m_adjusting = true; // the snap runs once at the end of init(), not from this resize
  this->resize(width + chrome.width(), height + chrome.height());
  m_adjusting = false;
}


// adjusts frame-index and frame to slider position
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


// fetch frame i into an owned QImage; all pixel access stays inside, while the refcounted PVideoFrame is still alive
bool avsViewer::grabFrame(const int& i, QImage& target)
{
  try {
    PClip clip = m_res.AsClip();    //get clip
    PVideoFrame frame = clip->GetFrame(i, m_env); // get frame number i
    if (!frame) {
      std::cerr << " couldn't show frame (no frame: " << i << ")" << std::endl;
      return false;
    }
    const unsigned char* data = frame->GetReadPtr();
    if (data == nullptr) {
      std::cerr << " couldn't show frame (no data: " << i << ")" << std::endl;
      return false;
    }
    // rows are padded to an aligned pitch, not packed at width*4, so pass the real pitch
    QImage wrapped(data, m_inf->width, m_inf->height, frame->GetPitch(), QImage::Format_RGB32);
    // Avisynth RGB is bottom-up; the flip also allocates, detaching us from the frame (flipped() is Qt 6.9+)
#if (QT_VERSION >= QT_VERSION_CHECK(6, 9, 0))
    target = wrapped.flipped();
#else
    target = wrapped.mirrored();
#endif
    return !target.isNull();
  } catch (AvisynthError err) { //catch AvisynthErrors
    std::cerr << "-> " << err.msg << std::endl;
  } catch (...) { //catch everything else
    std::cerr << "-> grabFrame - Unknown error (" << i << ")" << std::endl;
  }
  return false;
}

// decodes frame i and shows it; only this path touches Avisynth, re-scaling goes through updatePixmap()
void avsViewer::showFrame(const int& i)
{
  // i >= m_frameCount: valid indices are 0..m_frameCount-1, the old > let Next run one past the end
  if (m_env == nullptr || m_inf == nullptr || i < 0 || i >= m_frameCount || m_frameCount == 0) {
    return;
  }
  try {
    if (!this->grabFrame(i, m_currentImage)) { // already flipped and detached
      std::cerr << " could not get PVideoFrame data (" << i << ")" << std::endl;
      return;
    }
    m_current = i; //set m_current to i
    ui.frameHorizontalSlider->setSliderPosition(m_current); // adjust the slider position
    this->updateTitle();
    this->updatePixmap();
  } catch (...) {
    std::cerr << " couldn't show frame,..." << "(" << i << ")" << std::endl;
  }
}

// re-scales and re-displays the frame already held in m_currentImage
void avsViewer::updatePixmap()
{
  if (m_currentImage.isNull()) {
    return;
  }
  const int width = m_currentImage.width();
  const int height = m_currentImage.height();
  const double zoom = ui.zoomScaleDoubleSpinBox->value();
  const QString zoomHandling = ui.zoomHandlingComboBox->currentText();
  if (zoomHandling == "Fixed zoom") {
    m_zoom = zoom; // assign unconditionally: guarding on 'zoom > 1' meant the
  }                // factor could never be taken back down to 1.0
  m_showLabel->setText(QString());
  QPixmap map;
  if (!map.convertFromImage(m_currentImage)) {
    std::cerr << " couldn't convert image data to pixmap,.." << std::endl;
    return;
  }
  if (zoomHandling != "Fixed zoom") {
    // the area the frame is drawn into, not the whole window: the control rows below it are
    // 76px tall, so scaling into 'window minus a constant' overshot and clipped the bottom
    m_showLabel->setPixmap(map.scaled(this->frameAreaSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
  }
  else if (m_zoom != 1.0) {
    m_showLabel->setPixmap(
      map.scaled(int(width * m_zoom + 0.5), int(height * m_zoom + 0.5), Qt::KeepAspectRatio, Qt::FastTransformation));
  }
  else {
    m_showLabel->setPixmap(map);
  }
}

// only show the matrix when one was actually given, not a bare "(matrix: )"
QString avsViewer::matrixSuffix() const
{
  if (m_matrix.isEmpty()) {
    return QString();
  }
  return QString(" (matrix: %1)").arg(m_matrix);
}

void avsViewer::updateTitle()
{
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
  this->setWindowTitle(title + this->matrixSuffix());
}

void avsViewer::killEnv()
{
   qApp->processEvents();
  sendMessageToSever(QString("KILL environment"));
  this->cleanUp();
  if (!m_avsModified.isEmpty()) {
    std::cout << qPrintable(tr("deleting: %1").arg(m_avsModified)) << std::endl;
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

// allows to select a .avs file, starts the initialization
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
  m_providedInput = input; // refresh() rewrites from this, so it has to follow along
  this->reportInitError(this->init());
}

// refresh frame on resize event
void avsViewer::resizeEvent(QResizeEvent* event)
{
   QWidget::resizeEvent(event);
   // only re-scale: showFrame() would re-run the whole filter chain on every resize event
   this->updatePixmap();
   if (!m_adjusting) { // the timer coalesces a drag into a single aspect adjustment
     m_snapTimer->start();
   }
}

// leaving maximised or full screen hands the window size back to us, so re-fit the aspect
void avsViewer::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::WindowStateChange) {
    m_snapTimer->start();
  }
}

// shows the next frame
void avsViewer::on_nextPushButton_clicked()
{
  if (m_current < 0) {
    m_current = 0;
  }
  int next = m_current + 1;
  if (next >= m_frameCount) { // clamp like jumpForward does, instead of asking
    next = m_frameCount - 1;  // for a frame past the end
  }
  this->showFrame(next); // show next frame
}
// shows the previous frame
void avsViewer::on_previousPushButton_clicked()
{
  if (m_current < 1) {
    m_current = 1;
  }
  this->showFrame(m_current - 1); // show previous frame
}
// shows the frame of the index where the slider was released
void avsViewer::on_frameHorizontalSlider_sliderReleased()
{
  this->showFrame(ui.frameHorizontalSlider->sliderPosition()); // show frame for current slider position
}
