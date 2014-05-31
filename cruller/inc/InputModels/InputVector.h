#ifndef InputVector_h
#define InputVector_h

#include "Keyboard.h"

#include "boost/serialization/vector.hpp"
#include <vector>

namespace boost {namespace serialization {class access;}}

class InputVector {
  protected:
    std::vector<double> xvector, yvector, tvector;
  public:
   unsigned int Length(); 
   unsigned int AddPoint(double x, double y, double t = 0);
   double X(int i);
   double Y(int i);
   double T(int i);
   double SpatialLength();
   double TemporalLength();
   double DeltaPhi(unsigned int i);
   const char* StringForm(Keyboard& k);

  private:
    friend class boost::serialization::access;
    template<typename Archive> void serialize(Archive& ar, const unsigned int version) {
        ar & xvector & yvector & tvector;
    }
};

#endif
