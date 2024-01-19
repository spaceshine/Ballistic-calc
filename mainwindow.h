#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <Q3DScatter>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    float h0, v0, alpha, beta, u_value, gamma, mu, m, dt=0.01, h_end;
    float target_x, target_y, target_h;
    float anchor_alpha=0.0, anchor_beta=0.0, anchor_gamma=0.0;
    float step_alpha=1.0, step_beta=1.0, step_gamma=1.0;
    float range=1.0;
    int animation_counter;
    Q3DScatter *chart;
    QScatter3DSeries *series = new QScatter3DSeries, *start_point = new QScatter3DSeries, *target_point = new QScatter3DSeries;

    void start(bool);
    void animation();
    float get_alpha();
    float get_beta();
    float get_gamma();
    void progress(int);

private slots:
    void on_pushButton_start_clicked();

    void on_edt_alpha_valueChanged();

    void on_edt_beta_valueChanged();

    void on_edt_gamma_valueChanged();

    void on_precise_alpha_clicked();

    void on_precise_beta_clicked();

    void on_precise_gamma_clicked();

    void on_pushButton_optimal_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
