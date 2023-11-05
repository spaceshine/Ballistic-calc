#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "computation.h"
#include <QString>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_start_clicked()
{
    v0 = ui->edt_v0->text().toFloat();
    alpha = deg_to_rad(get_alpha());
    beta = deg_to_rad(get_beta());
    u_value = ui->edt_u->text().toFloat();
    gamma = deg_to_rad(get_gamma());
    mu = ui->edt_mu->text().toFloat();
    m = ui->edt_m->text().toFloat();
    dt = ui->edt_dt->text().toFloat();

    Vec v = Vec(v0, alpha, beta);
    Vec u = Vec(u_value, 0.0, gamma);

    Simulation result = compute(v, u, mu, m, dt);
    P end = P(result.points.back().x, result.points.back().y, 0.0);
    AVec v_end = result.v_end;
    float alpha_end = asin(abs(v_end.z/v_end.length()));

    ui->label_end->setText("(" + QString::number(end.x, 'f', 1) + ", " + QString::number(end.y, 'f', 1) + ")");
    ui->label_time->setText(QString::number(result.points.size()*dt, 'f', 1));
    ui->label_distance->setText(QString::number(AVec(end).length(), 'f', 1));
    ui->label_max_h->setText(QString::number(result.z_max, 'f', 1));
    ui->label_v_end->setText(QString::number(result.v_end.length(), 'f', 1));
    ui->label_alpha_end->setText(QString::number(rad_to_deg(alpha_end), 'f', 1) + "°");
}


float MainWindow::get_alpha(){return anchor_alpha + ui->edt_alpha->value()*step_alpha;}
float MainWindow::get_beta(){return anchor_beta + ui->edt_beta->value()*step_beta;}
float MainWindow::get_gamma(){return anchor_gamma + ui->edt_gamma->value()*step_gamma;}


void MainWindow::on_edt_alpha_valueChanged()
{
    ui->label_alpha->setText("Угол вертикальной наводки: (" + QString::number(get_alpha()) + "°)");
    //std::cout << anchor_alpha << " " << step_alpha << std::endl;
}


void MainWindow::on_edt_beta_valueChanged()
{
    ui->label_beta->setText("Угол горизонтальной наводки: (" + QString::number(get_beta()) + "°)");
}


void MainWindow::on_edt_gamma_valueChanged()
{
    ui->label_gamma->setText("Направление ветра: (" + QString::number(get_gamma()) + "°)");
}


// режим повышенной точности
void MainWindow::on_precise_alpha_clicked()
{
    if (ui->precise_alpha->isChecked()){
        anchor_alpha = get_alpha();
        step_alpha = 0.05;
        ui->edt_alpha->setValue(0);
    } else {
        ui->edt_alpha->setValue(int(get_alpha()));
        anchor_alpha = 0;
        step_alpha = 1.0;
        MainWindow::on_edt_alpha_valueChanged();
    }
}

void MainWindow::on_precise_beta_clicked()
{
    if (ui->precise_beta->isChecked()){
        anchor_beta = get_beta();
        step_beta = 0.05;
        ui->edt_beta->setValue(0);
    } else {
        ui->edt_beta->setValue(int(get_beta()));
        anchor_beta = 0;
        step_beta = 1.0;
        MainWindow::on_edt_beta_valueChanged();
    }
}

void MainWindow::on_precise_gamma_clicked()
{
    if (ui->precise_gamma->isChecked()){
        anchor_gamma = get_gamma();
        step_gamma = 0.1;
        ui->edt_gamma->setValue(0);
    } else {
        ui->edt_gamma->setValue(int(get_gamma()));
        anchor_gamma = 0;
        step_gamma = 1.0;
        MainWindow::on_edt_gamma_valueChanged();
    }
}

