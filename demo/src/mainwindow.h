#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QWidget>
#include <QComboBox>
#include <QMutex>
#include <QRadioButton>


class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

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


};

#endif // MAINWINDOW_H
