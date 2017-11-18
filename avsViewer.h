#ifndef AVSVIEWER_H
#define AVSVIEWER_H

#include "ui_viewer.h"
#include "avisynth.h"

#include <QObject>
#include <QImage>
#include <QWidget>
#include <QString>
#include <QPixmap>

class IScriptEnvironment;
class LocalSocketIpcServer;
class LocalSocketIpcClient;

class avsViewer : public QWidget
{
  Q_OBJECT
  public:
    avsViewer(QWidget *parent, QString path, double mult, bool cutSupport, QString cuts,
        QString ipcID, QString matrix);
    ~avsViewer();

  private:
    Ui::avsViewerClass ui;
    IScriptEnvironment* m_env;
    VideoInfo m_inf;
    PClip m_clip;
    int m_frameCount, m_current;
    QString m_currentInput, m_version, m_avsModified, m_inputPath;
    AVSValue m_res;
    double m_mult;
    QImage m_currentImage;
    bool m_cutSupport, m_dualView;
    int m_desktopWidth, m_desktopHeight;
    QString m_ipcID, m_currentContent, m_cuts;
    LocalSocketIpcServer* m_ipcServer;
    LocalSocketIpcClient* m_ipcClient;
    QString m_matrix;
    void showFrame(int frame);
    int init(int start = 0, const QString cuts = QString());
    int import(const char *inputFile);
    int invoke(const char *function);
    int invokeInternal(const char *function);
    int invokeImportInternal(const char *inputFile);
    void killEnv();
    bool isValidCut(int start, int end);
    void addCutList(QString list);
    QString fillUp(int number);
    void updateExistingMarks();
    void callMethod(const QString& typ, const QString& value, const QString &input);
    void cleanUp();

  private slots:
    void on_frameHorizontalSlider_valueChanged(int value);
    void on_nextPushButton_clicked();
    void on_previousPushButton_clicked();
    void on_frameHorizontalSlider_sliderReleased();
    void on_openAvsPushButton_clicked();
    void on_infoCheckBox_toggled();
    void on_histogramCheckBox_toggled();
    void on_saveImagePushButton_clicked();
    void on_setCutStartPushButton_clicked();
    void on_setCutEndPushButton_clicked();
    void on_addCutPushButton_clicked();
    void on_removeCutPushButton_clicked();
    void on_aspectRatioAdjustmentComboBox_currentIndexChanged(QString value);
    void fromConsoleReader(QString text);
    void receivedMessage(const QString& message);
    void on_jumpBackwardPushButton_clicked();
    void on_jumpForwardPushButton_clicked();
    void wheelEvent(QWheelEvent *event);
};

#endif // AVSVIEWER_H
