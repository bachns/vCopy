#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"

class MainWindow : public QMainWindow, Ui::MainWindowClass
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = Q_NULLPTR);

protected:
    void closeEvent(QCloseEvent* event);

signals:
    void stopPerform();

private slots:
    void selectSourceDir();
    void selectFileNamesFile();
    void selectDestinationDir();
    void extCheckBoxStateChanged(int state);
    void saveLogs();
    void perform();
    void stop();

    void preparing();
    void started();
    void stoped();
    void finished();
    void report(int quantity, int total);
    void notice(const QString& message);

private:
    QStringList readFileNames(const QString& fileNamesFile);
    bool mIsRunning = false;
};

#endif