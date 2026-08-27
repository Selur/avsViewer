#ifndef AVSVIEWER_H
#define AVSVIEWER_H

#include "ui_viewer.h"
#include "avisynth.h"
#include <QObject>
#include <QImage>
#include <QWidget>
#include <QString>
#include <QPixmap>
#include <QLabel>
#include <QFile>
#include <QLibrary>


class LocalSocketIpcServer;
class LocalSocketIpcClient;
class QResizeEvent;
class QThread;

class avsViewer : public QWidget
{
  Q_OBJECT
  public:
    avsViewer(QWidget *parent, const QString &path, const double &mult, const QString &ipcID, const QString &matrix);
    ~avsViewer();

  private:
    Ui::avsViewerClass ui;
    int m_frameCount, m_current;
    QString m_currentInput, m_avsModified, m_inputPath;
    QString m_lastError; // last Avisynth error text, shown by reportInitError()
    AVSValue m_res;
    double m_mult;
    QImage m_currentImage;
    bool m_dualView;
    int m_desktopWidth, m_desktopHeight;
    QString m_ipcID, m_currentScriptContent;
    LocalSocketIpcServer* m_ipcServer;
    /// on m_ipcClientThread - reach it only through sendMessageToSever()
    LocalSocketIpcClient* m_ipcClient;
    QThread* m_ipcClientThread;
    QString m_matrix;
    QLabel* m_showLabel;
    double m_zoom;
    int m_currentFrameWidth;
    int m_currentFrameHeight;
    IScriptEnvironment* m_env;
    const VideoInfo* m_inf;
    QString m_providedInput;
    QLibrary m_avsDLL;
    bool m_showOnly;
    void showFrame(const int &i);
    void updatePixmap(); // re-scale and re-display m_currentImage, no decoding
    void updateTitle();
    QString matrixSuffix() const;
    void reportInitError(int code);
    int init(int start = 0);
    bool initEnv();

    bool setRessource();
    bool setVideoInfo();
    bool invokeFunction(const QString& name);
    QString getColor() const;
    bool adjustScript(bool &invokeFFInfo);
    void killEnv();
    void callMethod(const QString& typ, const QString& value, const QString &input);
    void cleanUp();
    void refresh();
    void changeTo(const QString& input, const QString& value);
    void adjustWindowSize(const bool& adjust, const int& width, const int& height);
    void adjustLabelSize(const bool& adjust, const int& width, const int& height);
    void adjustToVideoInfo(const bool& scrolling, const bool& first, int& width, int& height, bool &changeLabelSize);
    void showVideoInfo();
    void initIPC();
    void sendMessageToSever(const QString& message);
    QString getCurrentInput(const QString& script);
    bool grabFrame(const int &i, QImage &target);
    void applyResolution(const QString& content, QString &newContent, double mult, const QString& resize);
    bool loadAvisynthDLL();

  protected:
    // event overrides, not slots; override keeps them bound if Qt changes a signature
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

  private slots:
    void on_frameHorizontalSlider_valueChanged(int value);
    void on_nextPushButton_clicked();
    void on_previousPushButton_clicked();
    void on_frameHorizontalSlider_sliderReleased();
    void on_openAvsPushButton_clicked();
    void on_infoCheckBox_toggled();
    void on_histogramCheckBox_toggled();
    void on_saveImagePushButton_clicked();
    void on_aspectRatioAdjustmentComboBox_currentTextChanged(const QString &value);
    void receivedMessage(const QString& message);
    void on_jumpBackwardPushButton_clicked();
    void on_jumpForwardPushButton_clicked();
    void on_jumpToPushButton_clicked();
    void on_jumpToStartPushButton_clicked();
    void on_jumpToEndPushButton_clicked();
    void on_zoomHandlingComboBox_currentTextChanged(const QString& value);
    void on_zoomScaleDoubleSpinBox_valueChanged(const double& value);
};

#endif // AVSVIEWER_H
