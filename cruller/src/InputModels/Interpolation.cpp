#include "InputModels/Interpolation.h"

#include "math.h"

//Helper functions
namespace {
    //This function simply returns the list of points from a quadratic bezier interpolation
    //of three control points.
    InputVector QuadraticBezierInterpolation(InputVector& iv, unsigned int Nsteps) {
        InputVector newIV;

        for(unsigned int i = 0; i <= Nsteps; i++)
        {
            //find the endpoints for the line that contains the new points (as a function of t)
            double px1 = iv.X(0)+(iv.X(1)-iv.X(0))*(i/double(Nsteps));
            double py1 = iv.Y(0)+(iv.Y(1)-iv.Y(0))*(i/double(Nsteps));
            double px2 = iv.X(1)+(iv.X(2)-iv.X(1))*(i/double(Nsteps));
            double py2 = iv.Y(1)+(iv.Y(2)-iv.Y(1))*(i/double(Nsteps));

            double newT = iv.T(0)+(iv.T(2)-iv.T(0))*(i/double(Nsteps));

            newIV.AddPoint(px1+(px2-px1)*(i/double(Nsteps)),py1+(py2-py1)*(i/double(Nsteps)),newT);
        }

        return newIV;
    }


    //Helper function that combines a vector of InputVectors into a single continuous InputVector.
    //The input vectors must be ordered correctly in the vector
    InputVector CombineInputVectors(std::vector<InputVector> IVvect) {
        InputVector newIV;

        for(unsigned int i = 0; i < IVvect.size(); i++) {
            if(i==0) {
                for(unsigned int j=0; j < IVvect[i].Length(); j++) {
                    newIV.AddPoint(IVvect[i].X(j),IVvect[i].Y(j),IVvect[i].T(j));
                }
            }

            else {
                for(unsigned int j=1; j < IVvect[i].Length(); j++) {
                    newIV.AddPoint(IVvect[i].X(j),IVvect[i].Y(j),IVvect[i].T(j));
                }
            }
        }

        return newIV;
    }


    //Helper function that returns the distance between point i in input vector iv and the next point in iv
    double DistanceToNextPoint(InputVector& iv, unsigned int i) {
        return sqrt(pow(iv.X(i+1)-iv.X(i),2) + pow(iv.Y(i+1)-iv.Y(i),2));
    }

    //Hermite basis functions for cubic spline interpolation
    inline double h00(double t) { return 2.0*t*t*t - 3.0*t*t + 1.0; }
    inline double h10(double t) { return t*t*t - 2.0*t*t + t; }
    inline double h01(double t) { return -2.0*t*t*t + 3.0*t*t; }
    inline double h11(double t) { return t*t*t - t*t; }
}


//Linear interpolation between points
InputVector SpatialInterpolation(InputVector& iv, unsigned int Nsteps) {
    const double length = iv.SpatialLength();
    const unsigned int points = iv.Length();
    const double steplength = length/(double(Nsteps)-1.0);

    InputVector newiv;
    newiv.AddPoint(iv.X(0), iv.Y(0), iv.T(0));
    for(unsigned int i = 1; i < Nsteps-1; i++) {
        double current_distance = steplength*double(i);

        unsigned int low_point = 0, high_point = 1;
        double low_distance = 0, high_distance = 0;

        while(true) {
            high_distance += sqrt( pow( iv.X(high_point)-iv.X(low_point), 2) + pow( iv.Y(high_point) - iv.Y(low_point), 2) );

            if(high_distance >= current_distance || high_point+1 == points) {
                break;
            }

            low_distance = high_distance;
            low_point = high_point;
            high_point++;
        }
        double high_weight = (current_distance - low_distance)/(high_distance-low_distance);
        if(high_distance == low_distance) { high_weight = 0.5; }
        double low_weight = 1.0 - high_weight;

        double newx = iv.X(high_point)*high_weight + iv.X(low_point)*low_weight;
        double newy = iv.Y(high_point)*high_weight + iv.Y(low_point)*low_weight;
        double newt = iv.T(high_point)*high_weight + iv.T(low_point)*low_weight;
        newiv.AddPoint(newx, newy, newt);
    }

    if(Nsteps > 1) {
        newiv.AddPoint(iv.X(points-1), iv.Y(points-1), iv.T(points-1));
    }

    return newiv;
}


//Cubic spline interpolation using the Hermite polynomial representation.
//Has the option to do monotonic interpolation between points.
InputVector HermiteCubicSplineInterpolationBase(InputVector& iv, unsigned int Nsteps, bool monotonic) {
    const unsigned int points = iv.Length();

    //if less than three letters don't do any fancy interpolation
    if(points <= 2)
        return SpatialInterpolation(iv, Nsteps);

    double *t = new double [points];
    double *y[2] = {new double [points], new double [points]};
    for(unsigned int i = 0; i <  points; i++) {
        t[i] = iv.T(i);
        y[0][i] = iv.X(i);
        y[1][i] = iv.Y(i);
    }
    //Slopes of the secant lines
    double *delta[2] = {new double [points-1], new double [points-1]};
    //Tangents at each data point (average of the secants)
    double *m[2] = {new double [points], new double [points]};

    for(unsigned int dimension = 0; dimension < 2; dimension++) {
        for(unsigned int i = 0; i <  points - 1; i++) {
            delta[dimension][i] = (y[dimension][i+1] - y[dimension][i])/(t[i+1] - t[i]);
        }
        //One-sided differences for the endpoints
        m[dimension][0] = delta[dimension][0];
        m[dimension][points - 1] = delta[dimension][points - 2];
        for(unsigned int i = 1; i <  points - 1; i++) {
            m[dimension][i] = 0.5*(delta[dimension][i-1] + delta[dimension][i]);
        }
        if(monotonic) {
            for(unsigned int i = 0; i <  points - 1; i++) {
                //Cases where the point is an extremum
                if( y[dimension][i] == y[dimension][i+1]
                        || (i > 0 &&
                           ((y[dimension][i] >= y[dimension][i-1] && y[dimension][i] >= y[dimension][i+1])
                           || (y[dimension][i] <= y[dimension][i-1] && y[dimension][i] <= y[dimension][i+1])))) {
                    m[dimension][i] = 0;
                    continue;
                }
                //Check for and eliminate overshoot
                const double alpha = m[dimension][i]/delta[dimension][i];
                const double beta = m[dimension][i+1]/delta[dimension][i];
                const double sum2 = alpha*alpha + beta*beta;
                if(sum2 > 9) {
                    const double tau = 3.0/sqrt(sum2);
                    m[dimension][i] = tau*alpha*delta[dimension][i];
                    m[dimension][i+1] = tau*beta*delta[dimension][i];

                }
            }
        }
    }

    //Create the new interpolated vector
    InputVector newiv;
    newiv.AddPoint(iv.X(0), iv.Y(0), iv.T(0));
    const double start_time = iv.T(0), end_time = iv.T(-1);
    const double total_time = end_time - start_time;
    unsigned int lower = 0;
    for(unsigned int i = 1; i < Nsteps-1; i++) {
        double current_time = start_time + total_time*double(i)/double(Nsteps - 1);
        while(iv.T(lower+1) < current_time) {
            lower++;
        }
        const unsigned int upper = lower + 1;
        const double h = iv.T(upper) - iv.T(lower);
        const double t = (current_time - iv.T(lower))/h;

        const double current_x = iv.X(lower)*h00(t) + h*m[0][lower]*h10(t) + iv.X(upper)*h01(t) + h*m[0][upper]*h11(t);
        const double current_y = iv.Y(lower)*h00(t) + h*m[1][lower]*h10(t) + iv.Y(upper)*h01(t) + h*m[1][upper]*h11(t);

        newiv.AddPoint(current_x, current_y, current_time);
    }
    if(Nsteps > 1) {
        newiv.AddPoint(iv.X(-1), iv.Y(-1), iv.T(-1));
    }

    delete [] t;
    for(unsigned int dimension = 0; dimension < 2; dimension++) {
        delete [] m[dimension];
        delete [] y[dimension];
        delete [] delta[dimension];
    }

    return newiv;
}


//Monotonic hermite cubic spline interpolation
InputVector MonotonicCubicSplineInterpolation(InputVector& iv, unsigned int Nsteps) {
    return HermiteCubicSplineInterpolationBase(iv, Nsteps, true);
}


//Normal Hermite cubic spline interpolation
InputVector HermiteCubicSplineInterpolation(InputVector& iv, unsigned int Nsteps) {
    return HermiteCubicSplineInterpolationBase(iv, Nsteps, false);
}


//New cubic spline interpolation function base.  Has the option to do normal cubic
//spline interpolation or to do the "modified" cubic spline interpolation which 
//constrains the first and last spline to be a straight line.
InputVector CubicSplineInterpolationBase(InputVector& iv, unsigned int Nsteps, bool mod) {
    const unsigned int nPoints = iv.Length();
    const unsigned int nSplines = nPoints-1;

    if (nPoints <= 2)
        return SpatialInterpolation(iv,Nsteps);

    //Algorithm coefficients
    double Dx[nPoints],cpx[nSplines],dpx[nPoints];
    double Dy[nPoints],cpy[nSplines],dpy[nPoints];

    //First step of tridiagonal matrix algorithm, a forward step to modify the coefficients
    for(unsigned int i=0; i < nPoints; i++) {
        if(i==0) {
            cpx[i] = 0.5;
            cpy[i] = 0.5;

            dpx[i] = 0.5*3.0*(iv.X(i+1)-iv.X(i));
            dpy[i] = 0.5*3.0*(iv.Y(i+1)-iv.Y(i));
        }
        else if (i < nPoints-1) {
            //Constrain the first and last segment of the spline are straight lines
            //and ensure that the derivatives match if mod==true
            if(mod==true && i==1) {
                    cpx[i] = 0;
                    cpy[i] = 0;

                    dpx[i] = (iv.X(i)-iv.X(i-1));
                    dpy[i] = (iv.Y(i)-iv.Y(i-1));
            }
            else if(mod==true && i==nPoints-2) {
                    dpx[i] = iv.X(i+1)-iv.X(i);
                    dpy[i] = iv.Y(i+1)-iv.Y(i);
            }
            else {
                cpx[i] = 1.0/(4.0-cpx[i-1]);
                cpy[i] = 1.0/(4.0-cpy[i-1]);

                dpx[i] = (3.0*(iv.X(i+1)-iv.X(i-1))-dpx[i-1])/(4.0-cpx[i-1]);
                dpy[i] = (3.0*(iv.Y(i+1)-iv.Y(i-1))-dpy[i-1])/(4.0-cpy[i-1]); 
            }
        }
        else {
            dpx[i] = (3.0*(iv.X(i)-iv.X(i-1))-dpx[i-1])/(2.0-cpx[i-1]);
            dpy[i] = (3.0*(iv.Y(i)-iv.Y(i-1))-dpy[i-1])/(2.0-cpy[i-1]); 
        }
    }

    //Backward substitution step of tridiagnoal matrix algorithm.
    //This obtains the value of the derivative of the interpolation curve at each point.
    if(nPoints > 0) {
        //In the modified algorithm, the first and last splines are lines which are already known
        //so we don't need to solve for them.  
        unsigned int i = (mod==true) ? nPoints-2 : nPoints-1;
        Dx[i] = dpx[i];
        Dy[i] = dpy[i];
        while(i > (mod==true) ? 1 : 0) {
             Dx[i-1] = dpx[i-1]-cpx[i-1]*Dx[i];
             Dy[i-1] = dpy[i-1]-cpy[i-1]*Dy[i];
             i--;
         }
    }

    //Create new InputVector by walking through each spline curve
    InputVector newIV;
    for(unsigned int i=0; i < nSplines; i++) {
        double newX, newY, newT, step;
        unsigned int nSteps = int(Nsteps*DistanceToNextPoint(iv,i)/iv.SpatialLength());

        for(unsigned int j=0; j < nSteps; j++) {
            if(nSteps==1) step = 1;
            else step = double(j)/double(nSteps);

            //Constrain first and last spline to be lines in the modified version
            if((i==0 || i==nSplines-1) && mod==true) {
                newX = iv.X(i) + (iv.X(i+1)-iv.X(i))*step;
                newY = iv.Y(i) + (iv.Y(i+1)-iv.Y(i))*step;
                newT = iv.T(i) + (iv.T(i+1)-iv.T(i))*step;
            }
            else { 
                newX = iv.X(i) + Dx[i]*step + (3*(iv.X(i+1)-iv.X(i))-2*Dx[i]-Dx[i+1])*pow(step,2) +
                    (2*(iv.X(i)-iv.X(i+1))+Dx[i]+Dx[i+1])*pow(step,3);
                newY = iv.Y(i) + Dy[i]*step + (3*(iv.Y(i+1)-iv.Y(i))-2*Dy[i]-Dy[i+1])*pow(step,2) + 
                    (2*(iv.Y(i)-iv.Y(i+1))+Dy[i]+Dy[i+1])*pow(step,3);            
                newT = iv.T(i)+(iv.T(i+1)-iv.T(i))*step; 
            }

            newIV.AddPoint(newX,newY,newT);
        }
    }
    newIV.AddPoint(iv.X(nSplines),iv.Y(nSplines),iv.T(nSplines));
    return newIV;
}


//Normal cubic spline interplation using the triangular matrix algorithm, should be 
//identical to the hermite cubic spline algorithm
InputVector CubicSplineInterpolation(InputVector& iv, unsigned int Nsteps) {
    return CubicSplineInterpolationBase(iv, Nsteps, false);
}


//Modified cubic spline interpolation.  The first and last splines are constrained
//to be straight lines, the points between are interpollated with a cubic spline.
InputVector ModCubicSplineInterpolation(InputVector& iv, unsigned int Nsteps) {
    if(iv.Length()==3)
        return CubicSplineInterpolationBase(iv, Nsteps, false);
    else
        return CubicSplineInterpolationBase(iv, Nsteps, true);
}


//Quadratic bezier interpollation (version 2)
InputVector BezierInterpolation(InputVector& iv, unsigned int Nsteps) {
    const unsigned int nPoints = iv.Length();
    const unsigned int nBezPoints = 3*nPoints - 4;

    //No fancy interpolation necessary for one or two letter words
    if (nPoints <= 2)
        return SpatialInterpolation(iv,Nsteps);

    //Create a new input vector with the bezier control points included
    InputVector bezIV;

    bezIV.AddPoint(iv.X(0),iv.Y(0),iv.T(0));
    for(unsigned int i = 1; i < nPoints-1; i++) {
        bezIV.AddPoint(iv.X(i)-(iv.X(i)-iv.X(i-1))/4,iv.Y(i)-(iv.Y(i)-iv.Y(i-1))/4,iv.T(i)-(iv.T(i)-iv.T(i-1))/4);
        bezIV.AddPoint(iv.X(i),iv.Y(i),iv.T(i));
        bezIV.AddPoint(iv.X(i)+(iv.X(i+1)-iv.X(i))/4,iv.Y(i)+(iv.Y(i+1)-iv.Y(i))/4,iv.T(i)+(iv.T(i+1)-iv.T(i))/4);
    }
    bezIV.AddPoint(iv.X(nPoints-1),iv.Y(nPoints-1),iv.T(nPoints-1));

    //Create a list of input vectors corresponding to the different segments of the swype pattern.
    //These will all be combined in end to form the final swype pattern
    std::vector<InputVector> IVlist;

    //The first segmens is just a straight line
    InputVector firstSeg;
    firstSeg.AddPoint(bezIV.X(0), bezIV.Y(0), bezIV.T(0));
    firstSeg.AddPoint(bezIV.X(1), bezIV.Y(1), bezIV.T(1));
    IVlist.push_back(firstSeg);

    //Create and add the quadratic bezier interpolated input vectors that form the middle section of the swype pattern
    for(unsigned int i = 2; i < nBezPoints; i+=3) {
        //Create the next input vector segment to be bezier interpolated
        InputVector bezSeg;
        bezSeg.AddPoint(bezIV.X(i-1),bezIV.Y(i-1),bezIV.T(i-1));
        bezSeg.AddPoint(bezIV.X(i),bezIV.Y(i),bezIV.T(i));
        bezSeg.AddPoint(bezIV.X(i+1),bezIV.Y(i+1),bezIV.T(i+1));

        int nSegSteps = int(Nsteps*((1.5*DistanceToNextPoint(bezIV,i-1))/bezIV.SpatialLength()));
        IVlist.push_back(QuadraticBezierInterpolation(bezSeg,nSegSteps));

        //Create the next linear segment that connects neighboring bezier segments
        InputVector linSeg;
        linSeg.AddPoint(bezIV.X(i+1),bezIV.Y(i+1),bezIV.T(i+1));
        linSeg.AddPoint(bezIV.X(i+2),bezIV.Y(i+2),bezIV.T(i+2));
        IVlist.push_back(linSeg);
    }

    //Combine all of the segments into one input vector
    InputVector combinedIV;
    combinedIV = CombineInputVectors(IVlist);
    return combinedIV;
}


//Very similar to the quadratic bezier interpolation algorith above.  The only difference is that
//the control points here are halfway between each letter.
InputVector BezierSloppyInterpolation(InputVector& iv, unsigned int Nsteps) {
    const unsigned int nPoints = iv.Length();
    const unsigned int nBezPoints = 2*nPoints - 1;

    //No fancy interpolation necessary for one or two letter words
    if (nPoints <= 2)
        return SpatialInterpolation(iv,Nsteps);

    //Create a new input vector with the bezier control points included
    InputVector bezIV;

    bezIV.AddPoint(iv.X(0),iv.Y(0),iv.T(0));
    for(unsigned int i = 1; i < nPoints; i++) {
        bezIV.AddPoint((iv.X(i)+iv.X(i-1))/2.0,(iv.Y(i)+iv.Y(i-1))/2.0,(iv.T(i)+iv.T(i-1))/2.0);
        bezIV.AddPoint(iv.X(i),iv.Y(i),iv.T(i));
    }

    //Create a list to hold the input veectors corresponding to various segments of the swype pattern
    std::vector<InputVector> IVlist;

    //The first segmens is just a straight line
    InputVector firstSeg;
    firstSeg.AddPoint(bezIV.X(0), bezIV.Y(0), bezIV.T(0));
    firstSeg.AddPoint(bezIV.X(1), bezIV.Y(1), bezIV.T(1));
    IVlist.push_back(firstSeg);

    //Create and add the quadratic bezier interpolated input vectors that form the middle section of the swype pattern
    for(unsigned int i = 2; i < nBezPoints-1; i+=2) {
        //Create the next input vector segment to be bezier interpolated
        InputVector bezSeg;
        bezSeg.AddPoint(bezIV.X(i-1),bezIV.Y(i-1),bezIV.T(i-1));
        bezSeg.AddPoint(bezIV.X(i),bezIV.Y(i),bezIV.T(i));
        bezSeg.AddPoint(bezIV.X(i+1),bezIV.Y(i+1),bezIV.T(i+1));

        int nSegSteps = int(Nsteps*((1.5*DistanceToNextPoint(bezIV,i-1))/bezIV.SpatialLength()));
        IVlist.push_back(QuadraticBezierInterpolation(bezSeg,nSegSteps));
    }

    //The final segment is also just a straight line
    InputVector lastSeg;
    lastSeg.AddPoint(bezIV.X(bezIV.Length()-2),bezIV.Y(bezIV.Length()-2),bezIV.T(bezIV.Length()-2));
    lastSeg.AddPoint(bezIV.X(bezIV.Length()-1),bezIV.Y(bezIV.Length()-1),bezIV.T(bezIV.Length()-1));
    IVlist.push_back(lastSeg);

    //Combine all of the segments into a single input vector
    InputVector combinedIV;
    combinedIV = CombineInputVectors(IVlist);
    return combinedIV;
}


