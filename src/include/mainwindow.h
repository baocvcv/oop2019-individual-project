#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QGroupBox>
#include <QTimer>
#include <QFileDialog>
#include <QStatusBar>

#include <string>
#include <vector>

#include "OnePassSynth.h"
#include "renderarea.h"
#include "Solver.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    MainWindow *ui;
    
    QWidget *window;
    
    // ShowPicDialog *dialog;
    
    QVBoxLayout *mainBox;
    QGridLayout *controlPanel;
    QHBoxLayout *mediaControl;
    
    QPushButton *selectInputBtn;
    QPushButton *runBtn;
    QPushButton *saveModelBtn;
    QPushButton *saveResultBtn;
    
    QSpinBox* widthInput;
    QSpinBox* heightInput;
    QSpinBox* timeInput;
    
    QPushButton *nextStepBtn;
    QPushButton *prevStepBtn;
    QPushButton *restartBtn;

    QStatusBar *bar;

    RenderArea *render;
    
    std::string filepath;
    
    // solver
    OnePassSynth *solver;

    // data from solver
    std::vector<std::vector<std::vector<int>>> gridData;
    std::vector<Node> sinkDispData;
    std::vector<std::vector<std::pair<bool, std::string>>> detectorData;

    int currentStep;
    
    void onSelectInput();
    void onRun();
    void onSaveModel();
    void onSaveResult();
    void onNextStep();
    void onPrevStep();
    void onRestart();    
    
    void paint();
};

#endif // MAINWINDOW_H
