//
//  typedef.h
//  waves
//
//  Created by Manuel MAGALHAES on 13/02/12.
//  Copyright 2012 Valkaari. All rights reserved.
//


#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_



typedef std::complex<double> complexe;




struct Vec2D {
	double x,y;
	Vec2D() {x=y=0.0;}
	Vec2D(double in) { x = y= in;}
	Vec2D(double ix, double iy){x = ix;y=iy;}
	
	
	
	double GetLengthSquared(void) const {
		return (x*x + y*y);	
	}
	double GetLength(void) const {
		return sqrt(x*x+y*y);
	}
	// multiply by a vector DOT product
	friend double operator * (const Vec2D &v1, const Vec2D &v2)
	{
		return (v1.x*v2.x + v1.y*v2.y);	
	}
    
	// multiply by a scalar
    friend Vec2D operator * (const Vec2D &v1,  double &scalar)
	{
		return Vec2D(v1.x*scalar , v1.y*scalar);	
	}
    friend Vec2D operator * (double &scalar, const Vec2D &v1)
    {
        return Vec2D (scalar* v1.x,scalar*v1.y);
    }
    
    
    
	friend Vec2D operator / (const Vec2D &v1,  double &scalar)
    {
        return Vec2D(v1.x/scalar, v1.y/scalar);
    }
    friend Vec2D operator / (double &scalar, const Vec2D &v1)
    {
        
        if ((v1.x != 0) && (v1.y != 0))
            return Vec2D(scalar/v1.x,scalar/v1.y);
        if ((v1.x == 0 ) && (v1.y ==0))
            return Vec2D(0,0);
        if (v1.x == 0)
            return Vec2D(0,scalar/v1.y);
        if (v1.y==0)
            return Vec2D(scalar/v1.x,0);
    }
    
    
    
    friend Vec2D operator + (const Vec2D &v1,  double &add)
    {
        return Vec2D(v1.x+add,v1.y+add);
    }
    friend const Vec2D operator - (const Vec2D &v1) 
    {
        return Vec2D(-v1.x,-v1.y);
    }
	
};

#endif // TYPEDEF_H
