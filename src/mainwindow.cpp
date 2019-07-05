#include "include/mainwindow.h"

#include <QPushButton>
#include <QPainter>
#include <string>
#include <iostream>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    window = new QWidget(this);
    mainBox = new QVBoxLayout;
    
    controlPanel = new QGridLayout;
    
    selectInputBtn = new QPushButton(this);
    runBtn = new QPushButton(this);
    saveModelBtn = new QPushButton(this);
    saveResultBtn = new QPushButton(this);
    nextStepBtn = new QPushButton(this);
    prevStepBtn = new QPushButton(this);
    restartBtn = new QPushButton(this);
    
    widthInput = new QSpinBox(this);
    heightInput = new QSpinBox(this);
    timeInput = new QSpinBox(this);

    auto labelWidth = new QLabel(this);
    auto labelHeight = new QLabel(this);
    auto labelTime = new QLabel(this);
    labelWidth->setText("Width");
    labelHeight->setText("Height");
    labelTime->setText("Time");
    
    selectInputBtn->setText("Select Input");
    runBtn->setText("Run");
    saveModelBtn->setText("Save Model");
    saveResultBtn->setText("Save Result");
    nextStepBtn->setText("Next");
    prevStepBtn->setText("Previous");
    restartBtn->setText("Restart");

    widthInput->setMinimum(1);
    heightInput->setMinimum(1);
    timeInput->setRange(1, 100);
    widthInput->setValue(3);
    heightInput->setValue(3);
    timeInput->setValue(5);

    controlPanel->addWidget(selectInputBtn, 1, 1);
    controlPanel->addWidget(saveModelBtn, 1, 2);
    controlPanel->addWidget(saveResultBtn, 1, 3);
    controlPanel->addWidget(labelWidth, 2, 1);
    controlPanel->addWidget(widthInput, 2, 2);
    controlPanel->addWidget(labelHeight, 2, 3);
    controlPanel->addWidget(heightInput, 2, 4);
    controlPanel->addWidget(labelTime, 3, 1);
    controlPanel->addWidget(timeInput, 3, 2);
    controlPanel->addWidget(runBtn, 3, 3, 1, 2);

    mainBox->addLayout(controlPanel);

    mediaControl = new QHBoxLayout;
    mediaControl->addWidget(nextStepBtn);
    mediaControl->addWidget(prevStepBtn);
    mediaControl->addWidget(restartBtn);
    mainBox->addLayout(mediaControl);

    render = new RenderArea;
    mainBox->addWidget(render);

    bar = new QStatusBar;
    mainBox->addWidget(bar);
    bar->showMessage("Select input file to run...");

    window->setLayout(mainBox);

    setCentralWidget(window);

    connect(selectInputBtn, &QPushButton::released, this, &MainWindow::onSelectInput);
    connect(runBtn, &QPushButton::released, this, &MainWindow::onRun);
    connect(saveModelBtn, &QPushButton::released, this, &MainWindow::onSaveModel);
    connect(saveResultBtn, &QPushButton::released, this, &MainWindow::onSaveResult);
    connect(nextStepBtn, &QPushButton::released, this, &MainWindow::onNextStep);
    connect(prevStepBtn, &QPushButton::released, this, &MainWindow::onPrevStep);
    connect(restartBtn, &QPushButton::released, this, &MainWindow::onRestart);

    solver = nullptr;
}

void MainWindow::onSelectInput(){
    filepath = QFileDialog::getOpenFileName(this, "Select Input", ".", "").toStdString();
    string msg = "File selected: " + filepath;
    bar->showMessage(msg.c_str());
}

void MainWindow::onRun() {
    if(solver != nullptr){
        delete solver;
    }
    solver = new OnePassSynth(filepath);

    int width = widthInput->value();
    int height = heightInput->value();
    int time = timeInput->value();

    bool is_solved = false;
    for(int t = time/2; t <= time && !is_solved; t++){
        if(solver->solve(width, height, t)){
            char msg[100];
            sprintf(msg, "Sat! w=%d h=%d t=%d. Showing status at t=0", width, height, t);
            bar->showMessage(msg);

            solver->print_solution();
            gridData = solver->get_grid();
            sinkDispData = solver->get_sink_dispenser_pos();
            detectorData = solver->get_detector_pos();

            is_solved = true;
            currentStep = 0;
            paint();
        }else{
            char msg[100];
            sprintf(msg, "Unsat! w=%d h=%d t=%d", width, height, t);
            bar->showMessage(msg);
        }
    }
}

void MainWindow::onSaveModel() {
    string savePath = filepath.substr(0, filepath.find_last_of('.')) + "_model.txt";
    cout << "Saving model to: " << savePath << endl;
    solver->get_solver().save_solver(savePath);
}

void MainWindow::onSaveResult() {
    string savePath = filepath.substr(0, filepath.find_last_of('.')) + "_result.txt";
    cout << "Saving result to: " << savePath << endl;
    solver->get_solver().save_solver(savePath);
}

void MainWindow::onNextStep() {
    if(currentStep < gridData.size()-1)
        currentStep++;
    paint();
    char msg[50];
    sprintf(msg, "Currently showing t=%d", currentStep);
    bar->showMessage(msg);
}

void MainWindow::onPrevStep() {
    if(currentStep > 0)
        currentStep--;
    paint();
    char msg[50];
    sprintf(msg, "Currently showing t=%d", currentStep);
    bar->showMessage(msg);
}

void MainWindow::onRestart() {
    currentStep = 0;
    paint();
    char msg[50];
    sprintf(msg, "Currently showing t=%d", currentStep);
    bar->showMessage(msg);
}

void MainWindow::paint(){
    render->gridData = gridData[currentStep];
    if(currentStep == 0){
        render->sinkDispData = sinkDispData;
        render->detectorData = detectorData;
    }

    render->do_update();
}

MainWindow::~MainWindow()
{
    delete ui;
}
