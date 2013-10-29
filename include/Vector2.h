#pragma once
#define _USE_MATH_DEFINES
#include <BWAPI.h>
#include <BWTA.h>
#include <math.h>

namespace ICEStarCraft
{
	class Vector2
	{
	public:
		Vector2(): _x(0), _y(0){}
		Vector2(double x, double y): _x(x), _y(y){}
		Vector2(const Vector2& v): _x(v._x), _y(v._y){}
    Vector2(const BWAPI::Position& p): _x(p.x()), _y(p.y()){}
		~Vector2(){}

		bool operator == (const Vector2& v) const {return _x==v._x && _y==v._y;}
		bool operator != (const Vector2& v) const {return _x!=v._x || _y!=v._y;}
    bool operator !() const {return _x!=0 && _y!=0;}

		Vector2 operator-() const {return Vector2(-_x, -_y);}

		Vector2 operator+(const Vector2& v) const {return Vector2(_x+v._x, _y+v._y);}
		Vector2 operator-(const Vector2& v) const {return Vector2(_x-v._x, _y-v._y);}
    BWAPI::Position operator+(const BWAPI::Position& p) const {return BWAPI::Position((int)_x+p.x(), (int)_y+p.y());}
		BWAPI::Position operator-(const BWAPI::Position& p) const {return BWAPI::Position((int)_x-p.x(), (int)_y-p.y());}
		Vector2 operator*(double d) const {return Vector2(d*_x, d*_y);}

		Vector2& operator+=(const Vector2& v){_x+=v._x; _y+=v._y; return *this;}
		Vector2& operator-=(const Vector2& v){_x-=v._x; _y-=v._y; return *this;}
    Vector2& operator+=(const BWAPI::Position& p){_x+=p.x(); _y+=p.y(); return *this;}
    Vector2& operator-=(const BWAPI::Position& p){_x-=p.x(); _y-=p.y(); return *this;}
		Vector2& operator*=(double d) {_x*=d; _y*=d; return *this;}


    // Dot product
    double operator*(const Vector2& v) const{return _x*v._x + _y*v._y;}
    // Cross product
    double operator^(const Vector2& v) const{return _x*v._y - _y*v._x;}

    //rotate
    Vector2& rotate(double c, double s) {
      double _t = c*_x - s*_y; 
      _y = s*_x + c*_y;
      _x = _t;
      return *this;
    }
    Vector2 rotate(double c, double s) const {return Vector2(c*_x-s*_y, s*_x+c*_y);}

		//normalize (to extract cos and sin)
		Vector2& normalize() {double _r = r(); _x/=_r; _y/=_r; return *this;}
		Vector2 normalize() const {double _r = r(); return Vector2(_x/_r, _y/_r);}


    Vector2& inv() {_x =  1.0/_x; _y = 1.0/_y; return *this;}
    Vector2 inv() const {return Vector2(1.0/_x, 1.0/_y);}

		//cos and sin
		double cos(const Vector2& v) const {return ((*this)*v)/(r()*v.r());}
		double sin(const Vector2& v) const {return ((*this)^v)/(r()*v.r());}
		//angle
		double angle(const Vector2& v) const {return 180 / M_PI * acos(this->cos(v));}
	
		double r2() const{return (_x)*(_x) + (_y)*(_y);}
		double r() const{return sqrt(_x*_x + _y*_y);}
		
		int approxLen();

		double& x(){return _x;}
		double& y(){return _y;}
		double x() const {return _x;}
		double y() const {return _y;}

	private:
		double _x;
		double _y;
	};
}