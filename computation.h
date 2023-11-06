#include <cmath>
#include <iostream>
#include <string>
#include <vector>
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


const AVec G = AVec(0.0, 0.0, -9.81);

struct Simulation{
    QScatterDataArray data;
    float z_max;
    AVec v_end;
};


Simulation compute(Vec v0, Vec u, float mu, float m, float dt){
    P r = P();
    AVec v = v0.to_avec();
    AVec v_;
    QScatterDataArray data;
    float z_max = 0;

    // std::cout << "SIMULATION" << std::endl;

    do{
        r = r + v*dt;

        //std::cout << r << std::endl;
        // используем вязкое трение при больших скоростях |F_c| = mu*(v^2)
        v_ = v + u.to_avec()*(-1); // скорость относительно воздуха (ветра)
        v = v + (G + v_*(-mu*v_.length()/m))*dt;

        data << QVector3D(r.x, r.z, r.y); // приколы Q3DScatter (y -> z)

        if (r.z > z_max){ // h_max
            z_max = r.z;
        }

    } while(r.z > 0);

    return Simulation{data, z_max, v};
}

