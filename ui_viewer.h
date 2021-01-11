/********************************************************************************
** Form generated from reading UI file 'viewer.ui'
**
** Created by: Qt User Interface Compiler version 6.0.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_VIEWER_H
#define UI_VIEWER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include "MarkSlider.h"
#include "MoveScrollArea.h"

QT_BEGIN_NAMESPACE

class Ui_avsViewerClass
{
public:
    QGridLayout *gridLayout_2;
    MoveScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QHBoxLayout *horizontalLayout;
    QLabel *avisynthResizerlabel;
    QComboBox *aspectRatioAdjustmentComboBox;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *openAvsPushButton;
    QSpacerItem *horizontalSpacer;
    QLabel *label;
    QSpinBox *jumpToSpinBox;
    QPushButton *jumpToPushButton;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *jumpToStartPushButton;
    QSpinBox *backwardSpinBox;
    QPushButton *jumpBackwardPushButton;
    QPushButton *previousPushButton;
    QLabel *frameNumberLabel;
    QPushButton *nextPushButton;
    QPushButton *jumpForwardPushButton;
    QSpinBox *forwardSpinBox;
    QPushButton *jumpToEndPushButton;
    QSpacerItem *horizontalSpacer_3;
    QCheckBox *infoCheckBox;
    QCheckBox *histogramCheckBox;
    QCheckBox *scrollingCheckBox;
    QPushButton *saveImagePushButton;
    MarkSlider *frameHorizontalSlider;

    void setupUi(QWidget *avsViewerClass)
    {
        if (avsViewerClass->objectName().isEmpty())
            avsViewerClass->setObjectName(QString::fromUtf8("avsViewerClass"));
        avsViewerClass->resize(1521, 764);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(avsViewerClass->sizePolicy().hasHeightForWidth());
        avsViewerClass->setSizePolicy(sizePolicy);
        gridLayout_2 = new QGridLayout(avsViewerClass);
        gridLayout_2->setSpacing(6);
        gridLayout_2->setContentsMargins(11, 11, 11, 11);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        gridLayout_2->setHorizontalSpacing(4);
        gridLayout_2->setVerticalSpacing(2);
        gridLayout_2->setContentsMargins(2, 0, 2, 2);
        scrollArea = new MoveScrollArea(avsViewerClass);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setFrameShadow(QFrame::Plain);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 1513, 718));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(scrollAreaWidgetContents->sizePolicy().hasHeightForWidth());
        scrollAreaWidgetContents->setSizePolicy(sizePolicy1);
        scrollArea->setWidget(scrollAreaWidgetContents);

        gridLayout_2->addWidget(scrollArea, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(4);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        avisynthResizerlabel = new QLabel(avsViewerClass);
        avisynthResizerlabel->setObjectName(QString::fromUtf8("avisynthResizerlabel"));
        sizePolicy.setHeightForWidth(avisynthResizerlabel->sizePolicy().hasHeightForWidth());
        avisynthResizerlabel->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(avisynthResizerlabel);

        aspectRatioAdjustmentComboBox = new QComboBox(avsViewerClass);
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->addItem(QString());
        aspectRatioAdjustmentComboBox->setObjectName(QString::fromUtf8("aspectRatioAdjustmentComboBox"));
        sizePolicy.setHeightForWidth(aspectRatioAdjustmentComboBox->sizePolicy().hasHeightForWidth());
        aspectRatioAdjustmentComboBox->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(aspectRatioAdjustmentComboBox);

        horizontalSpacer_2 = new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        openAvsPushButton = new QPushButton(avsViewerClass);
        openAvsPushButton->setObjectName(QString::fromUtf8("openAvsPushButton"));
        sizePolicy.setHeightForWidth(openAvsPushButton->sizePolicy().hasHeightForWidth());
        openAvsPushButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(openAvsPushButton);

        horizontalSpacer = new QSpacerItem(13, 13, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        label = new QLabel(avsViewerClass);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        jumpToSpinBox = new QSpinBox(avsViewerClass);
        jumpToSpinBox->setObjectName(QString::fromUtf8("jumpToSpinBox"));
        sizePolicy.setHeightForWidth(jumpToSpinBox->sizePolicy().hasHeightForWidth());
        jumpToSpinBox->setSizePolicy(sizePolicy);
        jumpToSpinBox->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(jumpToSpinBox);

        jumpToPushButton = new QPushButton(avsViewerClass);
        jumpToPushButton->setObjectName(QString::fromUtf8("jumpToPushButton"));
        sizePolicy.setHeightForWidth(jumpToPushButton->sizePolicy().hasHeightForWidth());
        jumpToPushButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(jumpToPushButton);

        horizontalSpacer_5 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_5);

        jumpToStartPushButton = new QPushButton(avsViewerClass);
        jumpToStartPushButton->setObjectName(QString::fromUtf8("jumpToStartPushButton"));

        horizontalLayout->addWidget(jumpToStartPushButton);

        backwardSpinBox = new QSpinBox(avsViewerClass);
        backwardSpinBox->setObjectName(QString::fromUtf8("backwardSpinBox"));
        sizePolicy.setHeightForWidth(backwardSpinBox->sizePolicy().hasHeightForWidth());
        backwardSpinBox->setSizePolicy(sizePolicy);
        backwardSpinBox->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        backwardSpinBox->setMaximum(10000);
        backwardSpinBox->setValue(100);

        horizontalLayout->addWidget(backwardSpinBox);

        jumpBackwardPushButton = new QPushButton(avsViewerClass);
        jumpBackwardPushButton->setObjectName(QString::fromUtf8("jumpBackwardPushButton"));
        sizePolicy.setHeightForWidth(jumpBackwardPushButton->sizePolicy().hasHeightForWidth());
        jumpBackwardPushButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(jumpBackwardPushButton);

        previousPushButton = new QPushButton(avsViewerClass);
        previousPushButton->setObjectName(QString::fromUtf8("previousPushButton"));
        sizePolicy.setHeightForWidth(previousPushButton->sizePolicy().hasHeightForWidth());
        previousPushButton->setSizePolicy(sizePolicy);
        previousPushButton->setAutoRepeat(true);

        horizontalLayout->addWidget(previousPushButton);

        frameNumberLabel = new QLabel(avsViewerClass);
        frameNumberLabel->setObjectName(QString::fromUtf8("frameNumberLabel"));
        sizePolicy.setHeightForWidth(frameNumberLabel->sizePolicy().hasHeightForWidth());
        frameNumberLabel->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(frameNumberLabel);

        nextPushButton = new QPushButton(avsViewerClass);
        nextPushButton->setObjectName(QString::fromUtf8("nextPushButton"));
        sizePolicy.setHeightForWidth(nextPushButton->sizePolicy().hasHeightForWidth());
        nextPushButton->setSizePolicy(sizePolicy);
        nextPushButton->setAutoRepeat(true);

        horizontalLayout->addWidget(nextPushButton);

        jumpForwardPushButton = new QPushButton(avsViewerClass);
        jumpForwardPushButton->setObjectName(QString::fromUtf8("jumpForwardPushButton"));
        sizePolicy.setHeightForWidth(jumpForwardPushButton->sizePolicy().hasHeightForWidth());
        jumpForwardPushButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(jumpForwardPushButton);

        forwardSpinBox = new QSpinBox(avsViewerClass);
        forwardSpinBox->setObjectName(QString::fromUtf8("forwardSpinBox"));
        sizePolicy.setHeightForWidth(forwardSpinBox->sizePolicy().hasHeightForWidth());
        forwardSpinBox->setSizePolicy(sizePolicy);
        forwardSpinBox->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        forwardSpinBox->setMaximum(10000);
        forwardSpinBox->setValue(100);

        horizontalLayout->addWidget(forwardSpinBox);

        jumpToEndPushButton = new QPushButton(avsViewerClass);
        jumpToEndPushButton->setObjectName(QString::fromUtf8("jumpToEndPushButton"));
        sizePolicy.setHeightForWidth(jumpToEndPushButton->sizePolicy().hasHeightForWidth());
        jumpToEndPushButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(jumpToEndPushButton);

        horizontalSpacer_3 = new QSpacerItem(0, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        infoCheckBox = new QCheckBox(avsViewerClass);
        infoCheckBox->setObjectName(QString::fromUtf8("infoCheckBox"));
        sizePolicy.setHeightForWidth(infoCheckBox->sizePolicy().hasHeightForWidth());
        infoCheckBox->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(infoCheckBox);

        histogramCheckBox = new QCheckBox(avsViewerClass);
        histogramCheckBox->setObjectName(QString::fromUtf8("histogramCheckBox"));
        sizePolicy.setHeightForWidth(histogramCheckBox->sizePolicy().hasHeightForWidth());
        histogramCheckBox->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(histogramCheckBox);

        scrollingCheckBox = new QCheckBox(avsViewerClass);
        scrollingCheckBox->setObjectName(QString::fromUtf8("scrollingCheckBox"));
        sizePolicy.setHeightForWidth(scrollingCheckBox->sizePolicy().hasHeightForWidth());
        scrollingCheckBox->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(scrollingCheckBox);

        saveImagePushButton = new QPushButton(avsViewerClass);
        saveImagePushButton->setObjectName(QString::fromUtf8("saveImagePushButton"));
        sizePolicy.setHeightForWidth(saveImagePushButton->sizePolicy().hasHeightForWidth());
        saveImagePushButton->setSizePolicy(sizePolicy);

        horizontalLayout->addWidget(saveImagePushButton);


        gridLayout_2->addLayout(horizontalLayout, 3, 0, 1, 2);

        frameHorizontalSlider = new MarkSlider(avsViewerClass);
        frameHorizontalSlider->setObjectName(QString::fromUtf8("frameHorizontalSlider"));
        frameHorizontalSlider->setOrientation(Qt::Horizontal);

        gridLayout_2->addWidget(frameHorizontalSlider, 2, 0, 1, 1);


        retranslateUi(avsViewerClass);

        QMetaObject::connectSlotsByName(avsViewerClass);
    } // setupUi

    void retranslateUi(QWidget *avsViewerClass)
    {
        avsViewerClass->setWindowTitle(QCoreApplication::translate("avsViewerClass", "avsViewer", nullptr));
        avisynthResizerlabel->setText(QCoreApplication::translate("avsViewerClass", "Aspect ratio resizer:", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(0, QCoreApplication::translate("avsViewerClass", "Bicubic", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(1, QCoreApplication::translate("avsViewerClass", "Bilinear", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(2, QCoreApplication::translate("avsViewerClass", "Blackman", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(3, QCoreApplication::translate("avsViewerClass", "Gauss", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(4, QCoreApplication::translate("avsViewerClass", "Lanczos", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(5, QCoreApplication::translate("avsViewerClass", "Lanczos4", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(6, QCoreApplication::translate("avsViewerClass", "Point", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(7, QCoreApplication::translate("avsViewerClass", "Sinc", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(8, QCoreApplication::translate("avsViewerClass", "Spline16", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(9, QCoreApplication::translate("avsViewerClass", "Spline36", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(10, QCoreApplication::translate("avsViewerClass", "Spline64", nullptr));
        aspectRatioAdjustmentComboBox->setItemText(11, QCoreApplication::translate("avsViewerClass", "None", nullptr));

#if QT_CONFIG(tooltip)
        openAvsPushButton->setToolTip(QCoreApplication::translate("avsViewerClass", "Open input .avs file", nullptr));
#endif // QT_CONFIG(tooltip)
        openAvsPushButton->setText(QCoreApplication::translate("avsViewerClass", "Open", nullptr));
#if QT_CONFIG(tooltip)
        label->setToolTip(QCoreApplication::translate("avsViewerClass", "<html><head/><body><p>Jump to a specific frame.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        label->setText(QCoreApplication::translate("avsViewerClass", "Jump to:", nullptr));
        jumpToPushButton->setText(QCoreApplication::translate("avsViewerClass", "Jump", nullptr));
#if QT_CONFIG(tooltip)
        jumpToStartPushButton->setToolTip(QCoreApplication::translate("avsViewerClass", "<html><head/><body><p>Jump to first frame.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        jumpToStartPushButton->setText(QCoreApplication::translate("avsViewerClass", "Start", nullptr));
        jumpBackwardPushButton->setText(QCoreApplication::translate("avsViewerClass", "Backward", nullptr));
#if QT_CONFIG(tooltip)
        previousPushButton->setToolTip(QCoreApplication::translate("avsViewerClass", "move one frame backward", nullptr));
#endif // QT_CONFIG(tooltip)
        previousPushButton->setText(QCoreApplication::translate("avsViewerClass", "Previous", nullptr));
        frameNumberLabel->setText(QCoreApplication::translate("avsViewerClass", "#", nullptr));
#if QT_CONFIG(tooltip)
        nextPushButton->setToolTip(QCoreApplication::translate("avsViewerClass", "move one frame backward", nullptr));
#endif // QT_CONFIG(tooltip)
        nextPushButton->setText(QCoreApplication::translate("avsViewerClass", "Next", nullptr));
        jumpForwardPushButton->setText(QCoreApplication::translate("avsViewerClass", "Forward", nullptr));
#if QT_CONFIG(tooltip)
        jumpToEndPushButton->setToolTip(QCoreApplication::translate("avsViewerClass", "<html><head/><body><p>Jump to last frame.</p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        jumpToEndPushButton->setText(QCoreApplication::translate("avsViewerClass", "End", nullptr));
#if QT_CONFIG(tooltip)
        infoCheckBox->setToolTip(QCoreApplication::translate("avsViewerClass", "see additional informations provided by FFInfo\n"
"- Frame Number\n"
"- Picture Type of input stream\n"
"- CFR/VFR Time", nullptr));
#endif // QT_CONFIG(tooltip)
        infoCheckBox->setText(QCoreApplication::translate("avsViewerClass", "Info", nullptr));
#if QT_CONFIG(tooltip)
        histogramCheckBox->setToolTip(QCoreApplication::translate("avsViewerClass", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">This mode will display three level-graphs on the right side of the video frame (which are called Histograms). This will show the distribution of the Y, U and V components in the current frame. </p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">The top graph displays the luma (Y) distribution of the frame, where the left side represents Y = 0 and the right side represents Y = 255. The valid CCIR601 range has been indicated by a slightly different color and Y = 128 has been "
                        "marked with a dotted line. The vertical axis shows the number of pixels for a given luma (Y) value. The middle graph displays the U component, and the bottom graph displays the V component. </p></body></html>", nullptr));
#endif // QT_CONFIG(tooltip)
        histogramCheckBox->setText(QCoreApplication::translate("avsViewerClass", "Histogram", nullptr));
        scrollingCheckBox->setText(QCoreApplication::translate("avsViewerClass", "scrolling", nullptr));
#if QT_CONFIG(tooltip)
        saveImagePushButton->setToolTip(QCoreApplication::translate("avsViewerClass", "Save current view as a picture", nullptr));
#endif // QT_CONFIG(tooltip)
        saveImagePushButton->setText(QCoreApplication::translate("avsViewerClass", "Save Image", nullptr));
    } // retranslateUi

};

namespace Ui {
    class avsViewerClass: public Ui_avsViewerClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_VIEWER_H
