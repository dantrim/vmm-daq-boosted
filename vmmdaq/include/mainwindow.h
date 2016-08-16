#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "data_handler.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    DataHandler* m_dataHandler;

signals :
    void stopDataGathering();

public slots :
    void startRun();
    void stopRun();
    void badOutputDir();
    void resetCounter();
    void updateCounts(int);
};

#endif // MAINWINDOW_H
