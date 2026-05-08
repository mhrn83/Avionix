#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings(new SettingsDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Avionix"));
    setWindowIcon(QIcon(":/img/img/flight-icon.jfif"));

    // Create Gauge Widgets
    m_OilPressureGauge = new QcGaugeWidget(this);
    m_OilPressureNeedle = new QcNeedleItem(m_OilPressureGauge);

    m_OilTemperatureGauge = new QcGaugeWidget(this);
    m_OilTemperatureNeedle = new QcNeedleItem(m_OilTemperatureGauge);

    m_FuelGauge = new QcGaugeWidget(this);
    m_FuelNeedle = new QcNeedleItem(m_FuelGauge);

    m_TorqueGauge = new QcGaugeWidget(this);
    m_TorqueNeedle = new QcNeedleItem(m_TorqueGauge);

    m_MotorSpeedGauge = new QcGaugeWidget(this);
    m_MotorSpeedNeedle = new QcNeedleItem(m_MotorSpeedGauge);

    // Color settings
    QList<QPair<QColor,float>> OilPressure_color, OilTemperature_color, Fuel_color, Torque_color, MotorSpeed_color;
    OilPressure_color.append({Qt::darkGreen,30});
    OilPressure_color.append({Qt::yellow,70});
    OilPressure_color.append({Qt::red,100});

    OilTemperature_color.append({Qt::darkGreen,30});
    OilTemperature_color.append({Qt::yellow,70});
    OilTemperature_color.append({Qt::red,100});

    Fuel_color.append({Qt::red,30});
    Fuel_color.append({Qt::yellow,70});
    Fuel_color.append({Qt::darkGreen,100});

    Torque_color.append({Qt::darkGreen,30});
    Torque_color.append({Qt::yellow,70});
    Torque_color.append({Qt::red,100});

    MotorSpeed_color.append({Qt::darkGreen,50});
    MotorSpeed_color.append({Qt::yellow,80});
    MotorSpeed_color.append({Qt::red,100});

    // Create Gauges
    createGauge(m_OilPressureGauge, m_OilPressureNeedle, "PSI", 100, OilPressure_color);
    createGauge(m_OilTemperatureGauge, m_OilTemperatureNeedle, " C°", 40, OilTemperature_color);
    createGauge(m_FuelGauge, m_FuelNeedle, "Liter", 80, Fuel_color);
    createGauge(m_TorqueGauge, m_TorqueNeedle, "N.m", 40, Torque_color);
    createGauge(m_MotorSpeedGauge, m_MotorSpeedNeedle, "Km/h", 100, MotorSpeed_color);

    // Create GroupBoxes and set titles
    OIL_PRESSURE_box.setTitle("OIL PRESSURE");
    //OIL_PRESSURE_box.setStyleSheet()
    OIL_TEMPERATURE_box.setTitle("OIL TEMPERATURE");
    FUEL_box.setTitle("FUEL");
    TORQUE_box.setTitle("TORQUE");
    MOTOR_SPEED_box.setTitle("MOTOR SPEED");

    // Create Layouts and add widgets
    OIL_PRESSURE_layout.addWidget(m_OilPressureGauge);
    OIL_TEMPERATURE_layout.addWidget(m_OilTemperatureGauge);
    FUEL_layout.addWidget(m_FuelGauge);
    TORQUE_layout.addWidget(m_TorqueGauge);
    MOTOR_SPEED_layout.addWidget(m_MotorSpeedGauge);

    OIL_PRESSURE_box.setLayout(&OIL_PRESSURE_layout);
    OIL_TEMPERATURE_box.setLayout(&OIL_TEMPERATURE_layout);
    FUEL_box.setLayout(&FUEL_layout);
    TORQUE_box.setLayout(&TORQUE_layout);
    MOTOR_SPEED_box.setLayout(&MOTOR_SPEED_layout);

    // Main Layout
    main_layout.addWidget(&OIL_PRESSURE_box);
    main_layout.addWidget(&OIL_TEMPERATURE_box);
    main_layout.addWidget(&FUEL_box);
    main_layout.addWidget(&TORQUE_box);
    main_layout.addWidget(&MOTOR_SPEED_box);

    main_widget.setLayout(&main_layout);

    ui->widget->addPage("Table Panel",&sensorTables);

    // Add main widget to widget container
    ui->widget->addPage("Gauge Panel", &main_widget);

    // Plot setup
    createPlot(&plot);
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%h:%m:%s");
    plot.xAxis->setTicker(timeTicker);
    plot.axisRect()->setupFullAxesBox();
    plot_layout.addWidget(&plot);
    plot_widget.setLayout(&plot_layout);
    plot.setContextMenuPolicy(Qt::CustomContextMenu);
    //plot_widget.setCursor(Qt::OpenHandCursor);
    ui->widget->addPage("Plot Panel", &plot_widget);

    // Tray Icon Setup
    createActions();
    createTrayIcon();
    m_trayIcon->show();
    m_trayIcon->setIcon(QIcon(":/img/img/flight-icon.jfif"));

    // Initialize connection status and message handler
    connection_status = false;
    messageHandler = new Process_message;

    // Create thread for message handler
    process_thread = new QThread;
    messageHandler->moveToThread(process_thread);
    process_thread->start();

    // Serial port setup
    m_port = new QSerialPort(this);

    // ScanTimer setup
    m_ScanTimer = new QTimer(this);
    m_ScanTimer->setInterval(20);

    // temp file for save data
    file = new QFile("savedData");

    connect(m_ScanTimer, &QTimer::timeout,
            this, &MainWindow::wait_for_data);

    connect(m_port, &QSerialPort::readyRead,
            this, &MainWindow::read_data);

    connect(this, &MainWindow::startProcess,
            messageHandler, &Process_message::processData);

    connect(messageHandler, &Process_message::updateGauge,
            this, &MainWindow::updateGaugePanel);

    connect(messageHandler, &Process_message::updatePlot,
            this, &MainWindow::updatePlotPanel);

    connect(messageHandler, &Process_message::updateTable,
            this, &MainWindow::updateTablePanel);

    connect(this, &MainWindow::saveSignal,
            this, &MainWindow::saveToFile);

    connect(&plot, SIGNAL(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*,QMouseEvent*)),
            this, SLOT(legendDoubleClick(QCPLegend*,QCPAbstractLegendItem*)));

    connect(&plot, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequest(QPoint)));

    connect(plot.xAxis, SIGNAL(rangeChanged(QCPRange)),
            plot.xAxis2, SLOT(setRange(QCPRange)));

    connect(plot.yAxis, SIGNAL(rangeChanged(QCPRange)),
            plot.yAxis2, SLOT(setRange(QCPRange)));
}


MainWindow::~MainWindow()
{
    disconnect(m_ScanTimer, &QTimer::timeout, this, &MainWindow::wait_for_data);
    disconnect(m_port, &QSerialPort::readyRead, this, &MainWindow::read_data);
    disconnect(this, &MainWindow::startProcess, messageHandler, &Process_message::processData);

    process_thread->quit();
    process_thread->wait();

    delete messageHandler;
    delete m_port;
    delete m_ScanTimer;
    delete m_trayIcon;
    delete m_trayIconMenu;
    delete process_thread;
    delete m_OilPressureGauge;
    delete m_OilPressureNeedle;
    delete m_OilTemperatureGauge;
    delete m_OilTemperatureNeedle;
    delete m_FuelGauge;
    delete m_FuelNeedle;
    delete m_TorqueNeedle;
    delete m_TorqueGauge;
    delete m_MotorSpeedNeedle;
    delete m_MotorSpeedGauge;
    delete m_minimizeAction;
    delete m_maximizeAction;
    delete m_restoreAction;
    delete m_quitAction;
    delete ui;
    delete m_settings;
}


void MainWindow::read_data()
{
    qDebug() << "data received";
    QByteArray data = m_port->readAll();
    messageHandler->setRawData(data);
    emit startProcess();

}

void MainWindow::on_connectButton_clicked()
{
    const SettingsDialog::Settings setting = m_settings->settings();

    if (connection_status) {
        qDebug() << "disconnecting!";
        m_port->close();
        file->close();
        connection_status = false;
        m_ScanTimer->stop();
        if(setting.notification){
            showMessage("Disconnected", m_port->portName() + " closed");
        }
        ui->connectButton->setText("Connect");
        m_OilPressureNeedle->setCurrentValue(0);
        m_OilTemperatureNeedle->setCurrentValue(0);
        m_FuelNeedle->setCurrentValue(0);
        m_TorqueNeedle->setCurrentValue(0);
        m_MotorSpeedNeedle->setCurrentValue(0);
        emit saveSignal();
        return;
    }

    m_port->setPortName(setting.name);
    m_port->setBaudRate(setting.baudRate);
    m_port->setDataBits(setting.dataBits);
    m_port->setParity(setting.parity);
    m_port->setStopBits(setting.stopBits);
    m_port->setFlowControl(setting.flowControl);

    if(m_port->open(QIODevice::ReadOnly)) {
        qDebug() << "SERIAL: OK!";
        file->open(QIODevice::WriteOnly | QIODevice::Text);
        connection_status = true;
        ui->connectButton->setText("Disconnect");
        m_ScanTimer->start();
        if(setting.notification){
            showMessage("Connected Successfully", m_port->portName());
        }
    } else {
        qDebug() << "SERIAL: ERROR!";
    }

}

void MainWindow::wait_for_data()
{
    if(m_port->isOpen()){
        m_port->waitForReadyRead(20);
    }
}

void MainWindow::updateGaugePanel()
{
    QMap<char,float> map = messageHandler->getFlags();
    m_OilPressureNeedle->setCurrentValue(map[char(OIL_PRESSURE)] / 10);
    m_OilTemperatureNeedle->setCurrentValue(map[char(OIL_TEMPERATURE)] / 10);
    m_FuelNeedle->setCurrentValue(map[char(FUEL)] / 10);
    m_TorqueNeedle->setCurrentValue(map[char(TORQUE)] / 10);
    m_MotorSpeedNeedle->setCurrentValue(map[char(MOTOR_SPEED)] / 10);
}

void MainWindow::updatePlotPanel()
{
    QMap<char,float> map = messageHandler->getFlags();
    static QTime time = QTime::currentTime();
    double key = time.elapsed()/1000.0;
    plot.graph(0)->addData(key, double(map[char(OIL_PRESSURE)]));
    plot.graph(1)->addData(key, double(map[char(OIL_TEMPERATURE)]));
    plot.graph(2)->addData(key, double(map[char(FUEL)]));
    plot.graph(3)->addData(key, double(map[char(TORQUE)]));
    plot.graph(4)->addData(key, double(map[char(MOTOR_SPEED)]));

    // make key axis range scroll with the data:
    plot.xAxis->rescale();
    plot.graph(0)->rescaleValueAxis(true, true);
    plot.graph(1)->rescaleValueAxis(true, true);
    plot.graph(2)->rescaleValueAxis(true, true);
    plot.graph(3)->rescaleValueAxis(true, true);
    plot.graph(4)->rescaleValueAxis(true, true);
    plot.xAxis->setRange(plot.xAxis->range().upper, 1, Qt::AlignRight);
    plot.replot();
}

void MainWindow::updateTablePanel()
{
    QMap<char,float> map = messageHandler->getFlags();
    QTextStream out(file);

    // Update Table
    sensorTables.setValue(map);

    // Write data to the file
    out << "Data,Value\n";

    out << "OIL PRESSURE" << "," << map[char(OIL_PRESSURE)] << "\n";
    out << "OIL TEMPERATURE" << "," << map[char(OIL_TEMPERATURE)] << "\n";
    out << "FUEL FLOW" << "," << map[char(FUEL_FLOW)] << "\n";
    out << "FUEL" << "," << map[char(FUEL)] << "\n";
    out << "EGT" << "," << map[char(EGT)] << "\n";
    out << "TORQUE" << "," << map[char(TORQUE)] << "\n";
    out << "INDICATED POWER" << "," << map[char(INDICATED_POWER)] << "\n";
    out << "FRICTIONAL POWER" << "," << map[char(FRICTIONAL_POWER)] << "\n";
    out << "THERMAL EFFICIENCY" << "," << map[char(THERMAL_EFFICIENCY)] << "\n";
    out << "AIR FUEL RATIO" << "," << map[char(AIR_FUEL_RATIO)] << "\n";
    out << "MOTOR SPEED" << "," << map[char(MOTOR_SPEED)] << "\n";
    out << "OUTPUT AIR SPEED" << "," << map[char(OUTPUT_AIR_SPEED)] << "\n";
    out << "VIBRATION" << "," << map[char(VIBRATION)] << "\n";
    out << "BODY TEMP" << "," << map[char(BODY_TEMP)] << "\n";
    out << "AIR TEMP" << "," << map[char(AIR_TEMP)] << "\n";

    out << "OIL PRESSURE SENSOR ERROR" << "," << map[char(OIL_PRESSURE_SENSOR_ERROR)] << "\n";
    out << "OIL TEMPERATURE SENSOR ERROR" << "," << map[char(OIL_TEMPERATURE_SENSOR_ERROR)] << "\n";
    out << "FUEL FLOW SENSOR ERROR" << "," << map[char(FUEL_FLOW_SENSOR_ERROR)] << "\n";
    out << "FUEL SENSOR ERROR" << "," << map[char(FUEL_SENSOR_ERROR)] << "\n";
    out << "EGT SENSOR ERROR" << "," << map[char(EGT_SENSOR_ERROR)] << "\n";
    out << "TORQUE SENSOR ERROR" << "," << map[char(TORQUE_SENSOR_ERROR)] << "\n";
    out << "INDICATED POWER SENSOR ERROR" << "," << map[char(INDICATED_POWER_SENSOR_ERROR)] << "\n";
    out << "FRICTIONAL POWER SENSOR ERROR" << "," << map[char(FRICTIONAL_POWER_SENSOR_ERROR)] << "\n";
    out << "THERMAL EFFICIENCY SENSOR ERROR" << "," << map[char(THERMAL_EFFICIENCY_SENSOR_ERROR)] << "\n";
    out << "AIR FUEL RATIO SENSOR ERROR" << "," << map[char(AIR_FUEL_RATIO_SENSOR_ERROR)] << "\n";
    out << "MOTOR SPEED SENSOR ERROR" << "," << map[char(MOTOR_SPEED_SENSOR_ERROR)] << "\n";
    out << "OUTPUT AIR SPEED SENSOR ERROR" << "," << map[char(OUTPUT_AIR_SPEED_SENSOR_ERROR)] << "\n";
    out << "VIBRATION SENSOR ERROR" << "," << map[char(VIBRATION_SENSOR_ERROR)] << "\n";
    out << "BODY TEMP SENSOR ERROR" << "," << map[char(BODY_TEMP_SENSOR_ERROR)] << "\n";
    out << "AIR TEMP SENSOR ERROR" << "," << map[char(AIR_TEMP_SENSOR_ERROR)] << "\n";
}

void MainWindow::createGauge(QcGaugeWidget* m_gauge, QcNeedleItem* m_needle, QString title, int range, QList<QPair<QColor,float>> colorList)
{
    m_gauge->addBackground(99);
    QcBackgroundItem *bkg1 = m_gauge->addBackground(92);
    bkg1->clearrColors();
    bkg1->addColor(0.1F,Qt::black);
    bkg1->addColor(1.0F,Qt::white);

    QcBackgroundItem *bkg2 = m_gauge->addBackground(88);
    bkg2->clearrColors();
    bkg2->addColor(0.1F,Qt::gray);
    bkg2->addColor(1.0F,Qt::darkGray);

    m_gauge->addArc(55);
    m_gauge->addDegrees(65)->setValueRange(0,range);
    m_gauge->addColorBand(50, colorList);

    m_gauge->addValues(80)->setValueRange(0,range);

    m_gauge->addLabel(55)->setText("× 10");
    m_gauge->addLabel(70)->setText(title);
    QcLabelItem *lab = m_gauge->addLabel(35);
    lab->setText("0");
    m_gauge->addNeedle(60, m_needle);
    m_needle->setLabel(lab);
    m_needle->setColor(Qt::white);
    m_needle->setValueRange(0,range);
    m_gauge->addBackground(7);
    m_gauge->addGlass(88);

}

void MainWindow::createPlot(QCustomPlot* plot)
{
    plot->addGraph();
    plot->graph()->setPen(QPen(Qt::blue));
    plot->graph(0)->setName("OIL PRESSURE");
    plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    plot->graph(0)->setVisible(true);
    plot->addGraph();
    plot->graph()->setPen(QPen(Qt::red));
    plot->graph(1)->setName("OIL TEMPERATURE");
    plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    plot->graph(1)->setVisible(false);
    plot->addGraph();
    plot->graph()->setPen(QPen(Qt::green));
    plot->graph(2)->setName("FUEL");
    plot->graph(2)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    plot->graph(2)->setVisible(false);
    plot->addGraph();
    plot->graph()->setPen(QPen(Qt::yellow));
    plot->graph(3)->setName("TORQUE");
    plot->graph(3)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    plot->graph(3)->setVisible(false);
    plot->addGraph();
    plot->graph()->setPen(QPen(Qt::darkCyan));
    plot->graph(4)->setName("MOTOR SPEED");
    plot->graph(4)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, QPen(Qt::black, 1.5), QBrush(Qt::white), 9));
    plot->graph(4)->setVisible(false);
    plot->axisRect()->setupFullAxesBox(true);
    plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
                          QCP::iSelectLegend | QCP::iSelectPlottables);
    plot->legend->setVisible(true);
    plot->legend->setBrush(QColor(255, 255, 255, 150));
    plot->yAxis->setRange(0, 1000);
    plot->xAxis->setLabel("time");
    plot->xAxis->setLabelColor(Qt::white);
    plot->yAxis->setLabel("sensor value");
    plot->yAxis->setLabelColor(Qt::white);
    plot->legend->setSelectableParts(QCPLegend::spItems);

    // move bars above graphs and grid below bars:
    plot->addLayer("abovemain", plot->layer("main"), QCustomPlot::limAbove);
    plot->addLayer("belowmain", plot->layer("main"), QCustomPlot::limBelow);
    plot->graph(0)->setLayer("abovemain");
    plot->graph(1)->setLayer("abovemain");
    plot->graph(2)->setLayer("abovemain");
    plot->graph(3)->setLayer("abovemain");
    plot->graph(4)->setLayer("abovemain");
    plot->xAxis->grid()->setLayer("belowmain");
    plot->yAxis->grid()->setLayer("belowmain");

    // set some pens, brushes and backgrounds:
    plot->xAxis->setBasePen(QPen(Qt::white, 1));
    plot->yAxis->setBasePen(QPen(Qt::white, 1));
    plot->xAxis->setTickPen(QPen(Qt::white, 1));
    plot->yAxis->setTickPen(QPen(Qt::white, 1));
    plot->xAxis->setSubTickPen(QPen(Qt::white, 1));
    plot->yAxis->setSubTickPen(QPen(Qt::white, 1));
    plot->xAxis->setTickLabelColor(Qt::white);
    plot->yAxis->setTickLabelColor(Qt::white);
    plot->xAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    plot->yAxis->grid()->setPen(QPen(QColor(140, 140, 140), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    plot->yAxis->grid()->setSubGridPen(QPen(QColor(80, 80, 80), 1, Qt::DotLine));
    plot->xAxis->grid()->setSubGridVisible(true);
    plot->yAxis->grid()->setSubGridVisible(true);
    plot->xAxis->grid()->setZeroLinePen(Qt::NoPen);
    plot->yAxis->grid()->setZeroLinePen(Qt::NoPen);
    plot->xAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    plot->yAxis->setUpperEnding(QCPLineEnding::esSpikeArrow);
    QLinearGradient plotGradient;
    plotGradient.setStart(0, 0);
    plotGradient.setFinalStop(0, 350);
    plotGradient.setColorAt(0, QColor(80, 80, 80));
    plotGradient.setColorAt(1, QColor(50, 50, 50));
    plot->setBackground(plotGradient);
    QLinearGradient axisRectGradient;
    axisRectGradient.setStart(0, 0);
    axisRectGradient.setFinalStop(0, 350);
    axisRectGradient.setColorAt(0, QColor(80, 80, 80));
    axisRectGradient.setColorAt(1, QColor(30, 30, 30));
    plot->axisRect()->setBackground(axisRectGradient);

}

void MainWindow::on_setting_clicked()
{
    m_settings->show();
}

void MainWindow::showMessage(QString title, QString body)
{
    m_trayIcon->showMessage(title, body, QIcon(":/img/img/flight-icon.jfif"), 5000);
}

void MainWindow::createActions()
{
    m_minimizeAction = new QAction(tr("Mi&nimize"), this);
    connect(m_minimizeAction, &QAction::triggered, this, &QWidget::hide);

    m_maximizeAction = new QAction(tr("Ma&ximize"), this);
    connect(m_maximizeAction, &QAction::triggered, this, &QWidget::showMaximized);

    m_restoreAction = new QAction(tr("&Restore"), this);
    connect(m_restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    m_quitAction = new QAction(tr("&Quit"), this);
    connect(m_quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
}

void MainWindow::createTrayIcon()
{
    // Create the context menu
    m_trayIconMenu = new QMenu(this);
    m_trayIconMenu->addAction(m_minimizeAction);
    m_trayIconMenu->addAction(m_maximizeAction);
    m_trayIconMenu->addAction(m_restoreAction);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(m_quitAction);

    // Create the tray icon
    m_trayIcon = new QSystemTrayIcon(this);

    // Set the icon for the tray
    m_trayIcon->setIcon(QIcon(":/img/img/flight-icon.jfif"));

    // Set the context menu for the tray icon
    m_trayIcon->setContextMenu(m_trayIconMenu);

    // Make the tray icon visible
    m_trayIcon->setVisible(true);
}

void MainWindow::legendDoubleClick(QCPLegend* legend, QCPAbstractLegendItem *selectedItem)
{
    Q_UNUSED(legend)

    if(selectedItem){
        for (int i=0; i< plot.graphCount(); ++i)
        {
            QCPGraph *graph = plot.graph(i);
            QCPPlottableLegendItem *item = plot.legend->itemWithPlottable(graph);
            if(selectedItem == item){
                plot.graph(i)->setVisible(true);
            }
            else{
                plot.graph(i)->setVisible(false);
            }
        }
        plot.replot();
    }
}

void MainWindow::contextMenuRequest(QPoint pos)
{
    QMenu *menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    if (plot.legend->selectTest(pos, false) >= 0)
    {
        menu->addAction("Move to top left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignLeft));
        menu->addAction("Move to top center", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignHCenter));
        menu->addAction("Move to top right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignTop|Qt::AlignRight));
        menu->addAction("Move to bottom right", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignRight));
        menu->addAction("Move to bottom left", this, SLOT(moveLegend()))->setData((int)(Qt::AlignBottom|Qt::AlignLeft));
    }

    menu->popup(plot.mapToGlobal(pos));
}

void MainWindow::moveLegend()
{
    if (QAction* contextAction = qobject_cast<QAction*>(sender())) // make sure this slot is really called by a context menu action, so it carries the data we need
    {
        bool ok;
        int dataInt = contextAction->data().toInt(&ok);
        if (ok)
        {
            plot.axisRect()->insetLayout()->setInsetAlignment(0, (Qt::Alignment)dataInt);
            plot.replot();
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    int currentTab = ui->widget->getCurrentTab();
    if(event->key() == Qt::Key_Left){
        if(currentTab > 0){
            ui->widget->setCurrentTab(currentTab - 1);
        }
        else{
            ui->widget->setCurrentTab(2);
        }
        return;
    }

    if(event->key() == Qt::Key_Right){
        if(currentTab < 2){
            ui->widget->setCurrentTab(currentTab + 1);
        }
        else{
            ui->widget->setCurrentTab(0);
        }
    }
}

void MainWindow::saveToFile()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save As"), "",
                                                    tr("CSV (*.csv)"));

    if (fileName.isEmpty())
        return;

    if (QFile::exists(fileName))
    {
        QFile::remove(fileName);
    }

    QFile::copy(QDir::currentPath() + "/savedData", fileName);
    QMessageBox::information(this, "Success", "File saved successfully!");
}



