#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <unistd.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_dataHandler(NULL)
{
    ui->setupUi(this);

    ui->outputDirectory_text->setText("/Users/dantrim/workarea/NSW/vmm-daq-boosted/output");

    //// tools ////
    m_dataHandler = new DataHandler(this);
    m_dataHandler->initialize();

    /// ------------------------------------------------------ ///
    //   widget connections
    /// ------------------------------------------------------ ///

    connect(ui->startRun_button, SIGNAL(clicked()), this, SLOT(startRun()));

    connect(m_dataHandler, SIGNAL(badOutputDir()), this, SLOT(badOutputDir()));

    connect(ui->clearCounts_button, SIGNAL(clicked()), this, SLOT(resetCounter()));

    connect(ui->stopRun_button, SIGNAL(clicked()), this, SLOT(stopRun()));

    connect(this, SIGNAL(stopDataGathering()), m_dataHandler, SLOT(endRun()));

    connect(m_dataHandler, SIGNAL(updateCountsSend(int)), this, SLOT(updateCounts(int)));

    connect(ui->loadDAQ_button, SIGNAL(clicked()), this, SLOT(loadDAQconfiguration()));

    connect(ui->loadMon_button, SIGNAL(clicked()), this, SLOT(updateMonitoringState()));


}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_dataHandler;
}

void MainWindow::loadDAQconfiguration()
{
    bool ok = false;
    std::string filename = "/Users/dantrim/workarea/NSW/vmm-daq-boosted/vmmdaq/readout_configuration/DAQ_config_TZ.xml";
    ok = m_dataHandler->loadMapping(filename);

    if(ok) {
        std::cout << "DAQ setup loaded successfully" << std::endl;
        ui->daqXml_text->setText(QString::fromStdString(filename));
    }
    else {
        std::cout << "DAQ setup unabled to be loaded" << std::endl;
    }
}

void MainWindow::updateMonitoringState()
{
    if(ui->loadMon_button->isChecked()) {
        m_dataHandler->setupMonitoring(true);
    }
    else {
        m_dataHandler->setupMonitoring(false);
    }
}



void MainWindow::updateCounts(int counts)
{
    ui->eventReceive_text->setText(QString::number(counts));
}

void MainWindow::badOutputDir()
{
    ui->outputDirectory_text->setStyleSheet("QLineEdit { background: red; }"); 
    sleep(1);

    ui->outputDirectory_text->setStyleSheet("QLineEdit { background: white; }");
}

void MainWindow::resetCounter()
{
    ui->eventReceive_text->setText("0");
}

// ------------------------------------------------------ //

void MainWindow::startRun()
{
    if(!m_dataHandler) {
        m_dataHandler = new DataHandler(this);
    }
    int count_to_stop_at = std::stoi(ui->eventCountStop_text->text().toStdString());
    bool ok_to_start = m_dataHandler->initializeRun(ui->writeNtuple_checkBox->isChecked(), ui->outputDirectory_text->text().toStdString(), count_to_stop_at, ui->mini2_checkBox->isChecked());
    if(!ok_to_start) {
        std::cout << "VMMDAQ INFO    Unable to start run" << std::endl;
        return;
    }
    m_dataHandler->fillRunProperties(3, 100, 80, 230, 300, 30, 3.125);
    m_dataHandler->startGathering();
}

void MainWindow::stopRun()
{
    emit stopDataGathering();
}

