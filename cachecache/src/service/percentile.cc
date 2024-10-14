#include "percentile.hh"
#include <algorithm>
#include <math.h>  

#include "folly/logging/xlog.h"
#include "folly/logging/LogLevel.h"

using namespace cachecache;

Percentile::Percentile(): Percentile(0.5) {}

Percentile::Percentile(double percentile):
    _p(percentile)
    , _n({0,0,0,0,0})
    , _ns({0,0,0,0,0})
    , _q({0,0,0,0,0})
    , _count(0) {}

Percentile::Percentile(Percentile&& other):
    _p(other._p)
    , _n(std::move(other._n))
    , _ns(std::move(other._ns))
    , _q(std::move(other._q))
    , _count(other._count) {
        other._p = 0;
        other._count = 0;
}

void Percentile::operator=(Percentile&& other) {

    this->_p = other._p;
    this->_n = std::move(other._n);
    this->_ns = std::move(other._ns);
    this->_q = std::move(other._q);
    this->_count = other._count;

    other._p = 0;
    other._count = 0;
}

void Percentile::reset() {
    this->_n = {0,0,0,0,0};
    this->_ns = {0,0,0,0,0};
    this->_q = {0,0,0,0,0};
    this->_count = 0;
}

void Percentile::setPercentile(double percentile) {
    this->reset();
    this->_p = percentile;
}

void Percentile::addValue(double value) {
    //XLOG(INFO, "Add value ", value, "- count = ", this->_count);
    if(this->_count < 5) {
        this->_q[this->_count] = value;
        this->_count++;
    
        if(this->_count == 5) {
            std::sort(this->_q.begin(), this->_q.end());

            for(int i = 0; i < 5; i++)
                this->_n[i] = i;

            this->_ns[0] = 0.0;
            this->_ns[1] = 2.0 * this->_p;
            this->_ns[2] = 4.0 * this->_p;
            this->_ns[3] = 2.0 + (2.0 * this->_p);
            this->_ns[4] = 4.0;
        }

        return;
    }

    int k;
    /*XLOG(INFO, "---------------");
    XLOG(INFO, "value: ", value);
    XLOG(INFO, "q [", this->_q[0], " ", this->_q[1], " ", this->_q[2], " ", this->_q[3], " ", this->_q[4], "]");
    XLOG(INFO, "n [", this->_n[0], " ", this->_n[1], " ", this->_n[2], " ", this->_n[3], " ", this->_n[4], "] (count: ", this->_count, ")");
    XLOG(INFO, "ns [", this->_ns[0], " ", this->_ns[1], " ", this->_ns[2], " ", this->_ns[3], " ", this->_ns[4], "]");
    XLOG(INFO, "---------------");*/

    if(value < this->_q[0]) {
        this->_q[0] = value;
        k = 0;
    } else if(value < this->_q[1]) {
        k = 0;
    } else if(value < this->_q[2]) {
        k = 1;
    } else if(value < this->_q[3]) {
        k = 2;
    } else if(value < this->_q[4]) {
        k = 3;
    } else {
        this->_q[4] = value;
        k = 3;
    }

    for(int i = k + 1; i < 5; i++) {
        this->_n[i]++;
    }

    this->_ns[1] = this->_count * this->_p / 2.0;
    this->_ns[2] = this->_count * this->_p;
    this->_ns[3] = this->_count * (1.0 + this->_p) / 2.0;
    this->_ns[4] = this->_count;

    for(int i = 1; i <= 3; i++) {
        double d = this->_ns[i] - this->_n[i];
        if( d >= 1 && this->_n[i + 1] - this->_n[i] > 1 ||
            d <= -1 && this->_n[i - 1] - this->_n[i] < -1)
        {
            int dInt = d > 0 ? 1 : (d < 0 ? -1 : 0);
            double qs = this->parabolic(i, dInt);
            if(!(this->_q[i - 1] < qs && qs < this->_q[i + 1]))
                qs = this->linear(i, dInt);
            this->_q[i] = qs;
            
            this->_n[i] += dInt;
        }
    }

    this->_count++;
}

double Percentile::parabolic(int i, double d) const {
    return this->_q[i] + (d / (this->_n[i + 1] - this->_n[i - 1])) * (
            (this->_n[i] - this->_n[i - 1] + d) * ((this->_q[i + 1] - this->_q[i]) / (this->_n[i + 1] - this->_n[i])) +
            (this->_n[i + 1] - this->_n[i] - d) * (this->_q[i] - this->_q[i - 1]) / (this->_n[i] - this->_n[i - 1])
        );
}

double Percentile::linear(int i, double d) const {
    return this->_q[i] + d * (this->_q[i + d] - this->_q[i]) / (this->_n[i + d] + this->_n[i]);
}

double Percentile::getCount() const {
    return this->_count;
}

double Percentile::getIndex() const {
    return this->_p;
}

double Percentile::getEstimation() const {
    return this->_q[2];
}

double Percentile::getPercentile() const {
    return this->_p;
}
