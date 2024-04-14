#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <Q3DScatter>


const float PI = 3.1415;

class P{
public:
    float x;
    float y;
    float z;
    P(){x=0.0; y=0.0; z=0.0;}
    P(float ax, float ay, float az){
        this->x = ax;
        this->y = ay;
        this->z = az;
    }
    ~P() = default;

    float distance_xy(P other){
        return sqrt(pow(this->x - other.x, 2) + pow(this->y - other.y, 2));
    }

    friend std::ostream& operator<<(std::ostream &out, const P &p){return p.print(out);}
    virtual std::ostream& print(std::ostream& out) const
    {
        out << x << " " << y << " " << z;
        return out;
    }
};


class AVec{
public:
    float x;
    float y;
    float z;
    AVec(){x=0.0; y=0.0; z=0.0;}
    AVec(float ax, float ay, float az){
        this->x = ax;
        this->y = ay;
        this->z = az;
    }
    AVec(P p2){
        this->x = p2.x;
        this->y = p2.y;
        this->z = p2.z;
    }
    ~AVec() = default;

    friend std::ostream& operator<<(std::ostream &out, const AVec &p){return p.print(out);}
    virtual std::ostream& print(std::ostream& out) const
    {
        out << x << " " << y << " " << z;
        return out;
    }

    float length(){
        return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2));
    }
};

P operator + (P p1, AVec av){
    return P(p1.x + av.x, p1.y + av.y, p1.z + av.z);
}

AVec operator + (AVec av1, AVec av2){
    return AVec(av1.x + av2.x, av1.y + av2.y, av1.z + av2.z);
}

AVec operator * (AVec av, float k){
    return AVec(av.x*k, av.y*k, av.z*k);
}


class Vec{
public:
    P p1, p2;
    Vec(){this->p1 = P(); this->p2 = P();}
    Vec(P p1, P p2){
        this->p1 = p1;
        this->p2 = p2;
    }
    Vec(float l, float alpha, float beta){ // alpha - горизонтальный угол, beta - вертикальный угол
        this->p1 = P();
        this->p2 = P(l*cos(alpha)*sin(beta), l*cos(alpha)*cos(beta), l*sin(alpha));
    }
    ~Vec() = default;

    friend std::ostream& operator<<(std::ostream &out, const Vec &p){return p.print(out);}
    virtual std::ostream& print(std::ostream& out) const
    {
        out << "(" << p1 << "), (" << p2 << ")";
        return out;
    }

    AVec to_avec(){
        return AVec(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z);
    }

    float length(){
        return sqrt(pow(p2.x-p1.x, 2) + pow(p2.y-p1.y, 2) + pow(p2.z-p1.z, 2));
    }

    Vec operator * (float k){
        return Vec(p1, p1 + to_avec()*k);
    }
};

float rad_to_deg(float rad){
    return rad*180/PI;
}

float deg_to_rad(float deg){
    return deg*PI/180;
}

const static AVec G = AVec(0.0, 0.0, -9.81);

struct Simulation{
    QScatterDataArray data;
    float z_max;
    AVec v_end;
};

AVec diff_velocity(AVec v, AVec u, float mu, float m){
    AVec v_ = v + u*(-1);
    return G + v_*(-mu*v_.length()/m);
}

Simulation compute(Vec v0, Vec u0, float mu, float m, float dt, float h0, float h_end=0){
    P r = P(0, 0, h0);
    AVec k0, k1, k2, k3;
    AVec v = v0.to_avec();
    AVec q0, q1, q2, q3;
    AVec u = u0.to_avec();
    QScatterDataArray data;
    float z_max = 0;
    bool cannot_bump=true;

    do{
        // Метод Рунге-Кутты 4 порядка
        k0 = v;
        q0 = diff_velocity(k0, u, mu, m);
        k1 = v + q0*(dt/2);
        q1 = diff_velocity(k1, u, mu, m);
        k2 = v + q1*(dt/2);
        q2 = diff_velocity(k2, u, mu, m);
        k3 = v + q2*(dt);
        q3 = diff_velocity(k3, u, mu, m);

        // Вычисляем новое значение скорости и радиус вектора
        v = v + (q0 + q1*2 + q2*2 + q3)*(dt/6);
        r = r + (k0 + k1*2 + k2*2 + k3)*(dt/6);

        data << QVector3D(r.x, r.z, r.y); // Особенности Q3DScatter (y -> z)

        if (r.z > z_max){ // h_max
            z_max = r.z;
        }

        if (r.z > h_end){
            cannot_bump = false;
        }

    } while((r.z > 0.0) && (r.z > h_end || cannot_bump));

    return Simulation{data, z_max, v};
}


struct ext_params{
    Vec u;
    float mu;
    float m;
    float dt;
    float h0;
    float h_end;
};
struct grad_params{
    float da;
    float db;
    float stepa;
    float stepb;
    long maxiter=2000;
};

// Функция вычисляет отклонение от целевой точки при заданных параметрах броска
float target_error(float v0, float alpha, float beta, P target, ext_params ep){
    P r = P(0, 0, ep.h0);
    AVec k0, k1, k2, k3;
    AVec v = Vec(v0, alpha, beta).to_avec();
    AVec q0, q1, q2, q3;
    AVec u = ep.u.to_avec();
    bool cannot_bump=true;

    do{
        // Метод Рунге-Кутты 4 порядка
        k0 = v;
        q0 = diff_velocity(k0, u, ep.mu, ep.m);
        k1 = v + q0*(ep.dt/2);
        q1 = diff_velocity(k1, u, ep.mu, ep.m);
        k2 = v + q1*(ep.dt/2);
        q2 = diff_velocity(k2, u, ep.mu, ep.m);
        k3 = v + q2*(ep.dt);
        q3 = diff_velocity(k3, u, ep.mu, ep.m);

        // Вычисляем новое значение скорости и радиус вектора
        v = v + (q0 + q1*2 + q2*2 + q3)*(ep.dt/6);
        r = r + (k0 + k1*2 + k2*2 + k3)*(ep.dt/6);

        if (r.z > ep.h_end){
            cannot_bump = false;
        }
    } while((r.z > 0.0) && (r.z > ep.h_end || cannot_bump));
    return r.distance_xy(target);
}

// Реализация алгоритма градиентного спуска для действительной функции от 2-х переменных
struct grad_return{
    float alpha;
    float beta;
    float func_value;
};

grad_return grad(float da, float db, float v0, float alpha, float beta, P target, ext_params ep){
    float I0 = target_error(v0, alpha, beta, target, ep);
    float dIda = (target_error(v0, alpha+da, beta, target, ep) - I0) / da; // Частная производная по alpha
    float dIdb = (target_error(v0, alpha, beta+db, target, ep) - I0) / db; // Частная производная по beta
    return {dIda, dIdb, I0};
}

float aborder_lower = -1.5708; // ~ -10 градусов
float aborder_upper = 1.5708;   // ~ 90 градусов
float bborder_lower = -1.5708;  // ~ -90 градусов
float bborder_upper = 1.5708;   // ~ 90 градусов

grad_return gradient_descent(grad_params gp, float v0, float alpha, float beta, P target, ext_params ep, std::function<void(int)> progress){
    float aa = alpha, bb = beta;
    grad_return gradI{alpha, beta, 1e6};
    // Итеративно спускаемся по градиенту (также вводим maxiter, чтобы алгоритм не работал
    // бесконечно при, например, невозможности попадания в цель)
    for(long i = 0; i < gp.maxiter; i++){
        gradI = grad(gp.da, gp.db, v0, aa, bb, target, ep); // Вычисляем градиент функции отклонения
        // Изменяем значение против градиента, чтобы минимизировать функцию
        aa = aa - gp.stepa * gradI.alpha;
        bb = bb - gp.stepb * gradI.beta;

        // Предотвращение выхода за границы допустимых углов
        if (aa < aborder_lower) {aa = aborder_lower;}
        if (aa > aborder_upper) {aa = aborder_upper;}
        if (bb < bborder_lower) {bb = bborder_lower;}
        if (bb > bborder_upper) {bb = bborder_upper;}

        // std::cout << "iteration[" << i << "] " << aa << " " << bb << " " << gradI.func_value << std::endl;
        progress(int(float(i)/gp.maxiter*100));
        // Критерий остановки
        if (gradI.func_value < 0.1f){
            return {aa, bb, gradI.func_value};
        }
    }
    return {aa, bb, gradI.func_value};
}

// Далее получаем поверхность отклика для анализа данных

QScatterDataArray grid_target_error(float alpha_min, float alpha_max, float beta_min, float beta_max, float angle_step, float v0, P target, ext_params ep){

    QScatterDataArray grid;
    for (float alpha = alpha_min; alpha < alpha_max; alpha += angle_step){
        for (float beta = beta_min; beta < beta_max; beta += angle_step){
            float t_error = target_error(v0, alpha, beta, target, ep);
            grid << QVector3D(alpha, t_error, beta); // Особенности Q3DScatter (y -> z)
        }
    }
    return grid;
}

























