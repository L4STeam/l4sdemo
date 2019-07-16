#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QWidget>
#include <QComboBox>
#include <QMutex>
#include <QRadioButton>

class DataGenerator;

class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void updateTA(bool ipc);
public slots:
    void updateDctcpclientCC(std::string);
    void updateCubicclientCC(std::string);
    void checkCC();

private:
    int checkIfUp(const char* ip);
    QRadioButton* becn;
    QRadioButton* bip;
    std::string dctcpclientCC;
    std::string cubicclientCC;
    QMutex dataMutex;
    DataGenerator *g;
};

#endif // MAINWINDOW_H
