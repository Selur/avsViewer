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

class LocalSocketIpcServer;
class LocalSocketIpcClient;
class QResizeEvent;

class avsViewer : public QWidget
{
  Q_OBJECT
  public:
    avsViewer(QWidget *parent, const QString &path, const double &mult, const QString &ipcID, const QString &matrix);
    ~avsViewer();

  private:
    Ui::avsViewerClass ui;
    int m_frameCount, m_current;
    QString m_currentInput, m_version, m_avsModified, m_inputPath;
    AVSValue m_res;
    PClip  m_clip;
    double m_mult;
    QImage m_currentImage;
    bool m_dualView;
    int m_desktopWidth, m_desktopHeight;
    QString m_ipcID, m_currentContent;
    LocalSocketIpcServer* m_ipcServer;
    LocalSocketIpcClient* m_ipcClient;
    QString m_matrix;
    QLabel* m_showLabel;
    double m_zoom;
    int m_currentFrameWidth;
    int m_currentFrameHeight;
    int m_fill;
    bool m_noAddBorders;
    IScriptEnvironment* m_env;
    void showFrame(const int &frame);
    int init(int start = 0);
    bool initEnv();
    const VideoInfo* m_inf;
    bool setRessource();
    bool setVideoInfo();
    bool invokeFunction(const QString& name);
    QString getColor() const;
    bool adjustScript(QString &input, bool &invokeFFInfo);
    void killEnv();
    QString fillUp(int number);
    void callMethod(const QString& typ, const QString& value, const QString &input);
    void cleanUp();
    void addBordersForFill(int &width);
    void cropForFill(QImage& image, int &width, const int &height);
    void refresh();
    void changeTo(const QString& input, const QString& value);
    void adjustWindowSize(const bool& adjust, const int& width, const int& height);
    void adjustLabelSize(const bool& adjust, const int& width, const int& height);
    void adjustToVideoInfo(const bool& scrolling, const bool& first, int& width, int& height, bool &changeLabelSize);
    void showVideoInfo();
    void initIPC();
  private slots:
    void on_frameHorizontalSlider_valueChanged(int value);
    void on_nextPushButton_clicked();
    void on_previousPushButton_clicked();
    void on_frameHorizontalSlider_sliderReleased();
    void on_openAvsPushButton_clicked();
    void on_infoCheckBox_toggled();
    void on_histogramCheckBox_toggled();
    void on_saveImagePushButton_clicked();
    void on_aspectRatioAdjustmentComboBox_currentIndexChanged(const QString &value);
    void fromConsoleReader(QString text);
    void receivedMessage(const QString& message);
    void on_jumpBackwardPushButton_clicked();
    void on_jumpForwardPushButton_clicked();
    void wheelEvent(QWheelEvent *event);
    void on_scrollingCheckBox_toggled();
    void resizeEvent(QResizeEvent* event);
    void on_jumpToPushButton_clicked();
    void on_jumpToStartPushButton_clicked();
    void on_jumpToEndPushButton_clicked();
};

#endif // AVSVIEWER_H
