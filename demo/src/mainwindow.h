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
    void updateDctcpclientCC(int value);
    void updateCubicclientCC(int value);
    void checkCC();

private:
    int checkIfUp(const char* ip);
    QRadioButton* becn;
    QRadioButton* bip;
    int dctcpclientCC;
    int cubicclientCC;
    QMutex dataMutex;
    DataGenerator *g;
};

#endif // MAINWINDOW_H
