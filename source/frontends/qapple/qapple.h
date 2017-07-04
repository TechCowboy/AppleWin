#ifndef QAPPLE_H
#define QAPPLE_H

#include "ui_qapple.h"

#include <QFileDialog>

class Emulator;

class QApple : public QMainWindow, private Ui::QApple
{
    Q_OBJECT

public:
    explicit QApple(QWidget *parent = 0);

protected:
    virtual void closeEvent(QCloseEvent * event);
    virtual void timerEvent(QTimerEvent *event);

private slots:
    void on_actionStart_triggered();

    void on_actionPause_triggered();

    void on_actionX1_triggered();

    void on_actionX2_triggered();

    void on_action4_3_triggered();

    void on_actionDisk_1_triggered();

    void on_actionDisk_2_triggered();

private:

    void setNextTimer(const int ms);
    void insertDisk(const int disk);

    QFileDialog myDiskFileDialog;

    QMdiSubWindow * myEmulatorWindow;
    Emulator * myEmulator;

    int myMSGap;
    int myCurrentGap;

    int myTimerID;
};

#endif // QAPPLE_H
