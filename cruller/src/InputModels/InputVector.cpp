#include "InputModels/InputVector.h"

#include "math.h"

#include <string>
using namespace std;

unsigned int InputVector::Length() {
    return xvector.size();
}

unsigned int InputVector::AddPoint(double x, double y, double t) {
    unsigned int i;
    for(i = 0; i < tvector.size(); i++) {
        if(t < tvector[i]) {
            break;
        }
    }

    xvector.insert((xvector.begin() + i), x);
    yvector.insert((yvector.begin() + i), y);
    tvector.insert((tvector.begin() + i), t);

    return Length();
}

double InputVector::X(unsigned int i) {
    return xvector[i];
}

double InputVector::Y(unsigned int i) {
    return yvector[i];
}

double InputVector::T(unsigned int i) {
    return tvector[i];
}

double InputVector::SpatialLength() {
    double l = 0;
    for(unsigned int i = 1; i < xvector.size(); i++) {
        l += sqrt( pow( xvector[i] - xvector[i-1], 2) + pow( yvector[i] - yvector[i-1], 2) );
    }
    return l;
}

double InputVector::TemporalLength() {
    return tvector[tvector.size()-1] - tvector[0];
}

double InputVector::DeltaPhi(unsigned int i) {
    //change in direction doesn't make sense at the ends
    if( i == 0 || i >= xvector.size() - 1 ) {
        return 0;
    }
    const double oldphi = atan2( (yvector[i]-yvector[i-1]), (xvector[i]-xvector[i-1]) );
    const double newphi = atan2( (yvector[i+1]-yvector[i]), (xvector[i+1]-xvector[i]) );

    return newphi - oldphi;
}

const char* InputVector::StringForm(Keyboard& k) {
    string s;

    unsigned int lastj = 0;
    for(unsigned int i = 0; i < xvector.size(); i++) {
        if( i>0 && k.GetKey(k.CharN(lastj)).IsInside(xvector[i], yvector[i]) ) {
            continue;
        }
        for(unsigned int j = 0; j < k.NKeys(); j++) {
            if( k.GetKey(k.CharN(j)).IsInside(xvector[i], yvector[i]) ) {
                s.push_back(k.CharN(j));
                lastj = j;
                break;
            }
        }
    }

    return s.c_str();
}
