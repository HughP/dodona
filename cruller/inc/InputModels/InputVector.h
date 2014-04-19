#ifndef InputVector_h
#define InputVector_h

#include "Keyboard.h"

#include <vector>
#include <utility>

class InputVector {
  protected:
    std::vector<double> xvector, yvector, tvector;
  public:
   unsigned int Length(); 
   unsigned int AddPoint(double x, double y, double t = 0);
   double X(unsigned int i);
   double Y(unsigned int i);
   double T(unsigned int i);
   double SpatialLength();
   double TemporalLength();
   double DeltaPhi(unsigned int i);
   const char* StringForm(Keyboard& k);
};

#endif
