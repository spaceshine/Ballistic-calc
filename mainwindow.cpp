#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "computation.h"
#include <QString>
#include <QRegExp>
#include <Q3DScatter>
#include <QTimer>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    chart = new Q3DScatter;
    QWidget *container = QWidget::createWindowContainer(chart);
    ui->vis_chart->addWidget(container, 1);
    chart->setAspectRatio(1.0);
    chart->setHorizontalAspectRatio(1.0);
    chart->setShadowQuality(QAbstract3DGraph::ShadowQualityNone); // отключаем тени
    chart->show();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_start_clicked()
{
    start(true);
}

void MainWindow::start(bool from_gui_inputs){
    target_x = ui->edt_target_x->text().toFloat();
    target_y = ui->edt_target_y->text().toFloat();
    target_h = ui->edt_target_h->text().toFloat();

    // Два способа ввода начальных данных: через пользовательский ввод и через специальную строку параметров (больше точность)
    if (from_gui_inputs && !ui->btn_optres->isChecked()){
        h0 = ui->edt_h0->text().toFloat();
        v0 = ui->edt_v0->text().toFloat();
        alpha = deg_to_rad(get_alpha());
        beta = deg_to_rad(get_beta());
        u_value = ui->edt_u->text().toFloat();
        gamma = deg_to_rad(get_gamma());
        mu = ui->edt_mu->text().toFloat();
        m = ui->edt_m->text().toFloat();
        dt = ui->edt_dt->text().toFloat();
    } else {
        QStringList params_list = ui->edt_optres->text().split(";");
        h0 = params_list[0].toFloat();
        v0 = params_list[1].toFloat();
        alpha = deg_to_rad(params_list[2].toFloat());
        beta = deg_to_rad(params_list[3].toFloat());
        u_value = params_list[4].toFloat();
        gamma = deg_to_rad(params_list[5].toFloat());
        mu = params_list[6].toFloat();
        m = params_list[7].toFloat();
        dt = params_list[8].toFloat();
    }

    Vec v = Vec(v0, alpha, beta);
    Vec u = Vec(u_value, 0.0, gamma);

    float h_end;
    if (ui->btn_is_user_h->isChecked()){
        h_end = target_h;
    } else{
        h_end = 0.0f;
    }

    ui->progressBar->setValue(10); // progressBar

    /* ОБЫЧНАЯ РАБОТА
    Simulation result = compute(v, u, mu, m, dt, h0, h_end);
    P end = P(result.data.back().x(), result.data.back().z(), 0.0);
    if (ui->btn_is_user_h->isChecked()){end.z = target_h;}
    AVec v_end = result.v_end;
    float alpha_end = asin(abs(v_end.z/v_end.length()));
    */

    // Кубок ЛФИ
    Simulation result = compute_spherical(v, m, dt, h0);
    P end = P(result.data.back().x(), result.data.back().z(), 0.0);
    if (ui->btn_is_user_h->isChecked()){end.z = target_h;}
    AVec v_end = result.v_end;
    float alpha_end = asin(abs(v_end.z/v_end.length()));

    ui->progressBar->setValue(90); // progressBar

    ui->label_end->setText("(" + QString::number(end.x, 'f', 1) + ", " + QString::number(end.y, 'f', 1) + ")");
    ui->label_time->setText(QString::number(result.data.size()*dt, 'f', 1));
    ui->label_distance->setText(QString::number(AVec(end).length(), 'f', 1));
    ui->label_max_h->setText(QString::number(result.z_max, 'f', 1));
    ui->label_v_end->setText(QString::number(result.v_end.length(), 'f', 1));
    ui->label_alpha_end->setText(QString::number(rad_to_deg(alpha_end), 'f', 1) + "°");

    // Убираем предыдущий график
    if (! ui->btn_fixed->isChecked()){
        for (auto ser : chart->seriesList()){
            chart->removeSeries(ser);
        }
    } else {
        chart->seriesList().at(0)->setBaseColor(Qt::green);
    }

    // Добавляем точки
    series = new QScatter3DSeries;
    series->dataProxy()->addItems(result.data);
    series->setBaseColor(Qt::red);
    series->setSingleHighlightColor(Qt::green);
    chart->addSeries(series);

    grid_mode = false;
    animation_counter = 0;
    animation_launched += 1;
    QTimer::singleShot(50, this, [this]() { animation(); } );

    // Добавляем точку старта на график отдельным цветом
    start_point = new QScatter3DSeries;
    QScatterDataArray start_p_data;
    //start_p_data << QVector3D(0.0, h0, 0.0); ОБЫЧНАЯ РАБОТА
    start_p_data << QVector3D(-h0, 0.0, 0.0); // КУБОК ЛФИ
    start_point->setBaseColor(Qt::black);
    start_point->setItemSize(series->itemSize()*1.2f);
    start_point->dataProxy()->addItems(start_p_data);
    chart->addSeries(start_point);

    // Добавляем точку цели на график отдельным цветом
    target_point = new QScatter3DSeries;
    QScatterDataArray target_p_data;
    target_p_data << QVector3D(target_x, target_h, target_y);
    target_point->setBaseColor(Qt::darkRed);
    target_point->setItemSize(series->itemSize()*1.2f);
    target_point->dataProxy()->addItems(target_p_data);
    chart->addSeries(target_point);

    // Настраиваем оси и показываем график
    float new_range = std::max(std::max(abs(end.x), abs(end.y)), abs(result.z_max));
    new_range = std::max(std::max(abs(target_x), abs(target_y)), new_range);
    if (ui->btn_fixed->isChecked()){
        range = std::max(range, new_range);
    } else {
        range = new_range;
    }
    chart->axisX()->setRange(-range, range);
    chart->axisY()->setRange(0, range);
    chart->axisZ()->setRange(0, range);
    chart->setAspectRatio(2);
    chart->setHorizontalAspectRatio(2);

    ui->progressBar->setValue(100); // progressBar

    chart->show();
}

void MainWindow::animation()
{
    if (animation_counter > series->dataProxy()->itemCount() || animation_launched > 1 || grid_mode){
        series->setSelectedItem(series->dataProxy()->itemCount()-1);
        animation_launched -= 1;
        return;
    }
    series->setSelectedItem(animation_counter);
    //std::cout << animation_counter << std::endl;
    animation_counter+=std::max(int(0.05/dt), 1); // Шаг анимации - 0.05 секунды
    QTimer::singleShot(50, [this]() { animation(); } );
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

void MainWindow::progress(int procents){
    ui->progressBar->setValue(procents);
}


void MainWindow::on_pushButton_optimal_clicked()
{
    // Два способа ввода начальных данных: через пользовательский ввод и через специальную строку параметров (больше точность)
    if (!ui->btn_optres->isChecked()){
        h0 = ui->edt_h0->text().toFloat();
        v0 = ui->edt_v0->text().toFloat();
        alpha = deg_to_rad(get_alpha());
        beta = deg_to_rad(get_beta());
        u_value = ui->edt_u->text().toFloat();
        gamma = deg_to_rad(get_gamma());
        mu = ui->edt_mu->text().toFloat();
        m = ui->edt_m->text().toFloat();
        dt = ui->edt_dt->text().toFloat();
    } else {
        QStringList params_list = ui->edt_optres->text().split(";");
        h0 = params_list[0].toFloat();
        v0 = params_list[1].toFloat();
        alpha = deg_to_rad(params_list[2].toFloat());
        beta = deg_to_rad(params_list[3].toFloat());
        u_value = params_list[4].toFloat();
        gamma = deg_to_rad(params_list[5].toFloat());
        mu = params_list[6].toFloat();
        m = params_list[7].toFloat();
        dt = params_list[8].toFloat();
    }
    target_x = ui->edt_target_x->text().toFloat();
    target_y = ui->edt_target_y->text().toFloat();
    target_h = ui->edt_target_h->text().toFloat();
    if (ui->btn_is_user_h->isChecked()){
        h_end = target_h;
    }
    Vec u = Vec(u_value, 0.0, gamma);

    ui->progressBar->setValue(0); // progressBar

    ext_params ep;
    ep.dt = dt;
    ep.m = m;
    ep.mu = mu;
    ep.u = u;
    ep.h0 = h0;
    ep.h_end = h_end;

    grad_params gp;
    gp.da = ui->edt_da->text().toFloat();
    gp.db = ui->edt_db->text().toFloat();
    gp.stepa = ui->edt_astep->text().toFloat();
    gp.stepb = ui->edt_bstep->text().toFloat();

    grad_return opt_params;
    opt_params = gradient_descent(gp, v0, alpha, beta, P(target_x, target_y, target_h), ep, [this](int procents){progress(procents);});
    //std::cout << opt_params.alpha << " " << opt_params.beta << " " << opt_params.func_value << std::endl;

    ui->edt_alpha->setValue(rad_to_deg(opt_params.alpha));
    ui->edt_beta->setValue(rad_to_deg(opt_params.beta));
    QString params_string = QString::number(h0) + ";" + QString::number(v0) + ";" + QString::number(rad_to_deg(opt_params.alpha), 'g', 5) + ";" + QString::number(rad_to_deg(opt_params.beta), 'g', 5) + "; "
            + QString::number(u_value) + ";" + QString::number(rad_to_deg(gamma), 'g', 5) + ";" + QString::number(mu) + ";" + QString::number(m) + ";" + QString::number(dt);
    ui->edt_optres->setText(params_string);

    //std::cout << params_string.toStdString() << std::endl;

    start(false);
}


void MainWindow::on_btn_grid_clicked()
{
    grid_mode = true;
    target_x = ui->edt_target_x->text().toFloat();
    target_y = ui->edt_target_y->text().toFloat();
    target_h = ui->edt_target_h->text().toFloat();

    // Два способа ввода начальных данных: через пользовательский ввод и через специальную строку параметров (больше точность)
    if (!ui->btn_optres->isChecked()){
        h0 = ui->edt_h0->text().toFloat();
        v0 = ui->edt_v0->text().toFloat();
        alpha = deg_to_rad(get_alpha());
        beta = deg_to_rad(get_beta());
        u_value = ui->edt_u->text().toFloat();
        gamma = deg_to_rad(get_gamma());
        mu = ui->edt_mu->text().toFloat();
        m = ui->edt_m->text().toFloat();
        dt = ui->edt_dt->text().toFloat();
    } else {
        QStringList params_list = ui->edt_optres->text().split(";");
        h0 = params_list[0].toFloat();
        v0 = params_list[1].toFloat();
        alpha = deg_to_rad(params_list[2].toFloat());
        beta = deg_to_rad(params_list[3].toFloat());
        u_value = params_list[4].toFloat();
        gamma = deg_to_rad(params_list[5].toFloat());
        mu = params_list[6].toFloat();
        m = params_list[7].toFloat();
        dt = params_list[8].toFloat();
    }

    Vec v = Vec(v0, alpha, beta);
    Vec u = Vec(u_value, 0.0, gamma);

    float h_end;
    if (ui->btn_is_user_h->isChecked()){
        h_end = target_h;
    } else{
        h_end = 0.0f;
    }

    ui->progressBar->setValue(10); // progressBar

    // float alpha_min, float alpha_max, float beta_min, float beta_max, float angle_step, float v0, P target, ext_params ep
    QScatterDataArray grid = grid_target_error(deg_to_rad(1.0), deg_to_rad(90.0), -deg_to_rad(90.0), deg_to_rad(90.0), deg_to_rad(0.5), v0, P{target_x, target_y, target_h}, ext_params{u, mu, m, dt, h0, h_end});
    // struct ext_params{Vec u;float mu;float m;float dt;float h0;float h_end;};


    ui->progressBar->setValue(90); // progressBar

    // Убираем предыдущий график
    if (! ui->btn_fixed->isChecked()){
        for (auto ser : chart->seriesList()){
            chart->removeSeries(ser);
        }
    } else {
        chart->seriesList().at(0)->setBaseColor(Qt::green);
    }

    // Добавляем точки
    series = new QScatter3DSeries;
    series->dataProxy()->addItems(grid);
    series->setBaseColor(Qt::green);
    series->setSingleHighlightColor(Qt::red);
    chart->addSeries(series);

    // Настраиваем оси и показываем график
//    chart->axisX()->setRange(-range, range);
//    chart->axisY()->setRange(0, range);
//    chart->axisZ()->setRange(0, range);
    chart->setAspectRatio(2);
    chart->setHorizontalAspectRatio(2);

    ui->progressBar->setValue(100); // progressBar

    chart->show();
}

