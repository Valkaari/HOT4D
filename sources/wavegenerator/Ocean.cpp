//
//  Ocean.cpp
//  waves
//
//  Created by Manuel MAGALHAES on 10/02/12.
//  Copyright 2012 Valkaari. All rights reserved.
//


#include "math.h"
#include <stdio.h>
#include "c4d.h"
//#include "ge_vector.h"


#include "Ocean.h"


#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 
#endif

#include "ImathRandom.h"




drw::OceanContext::~OceanContext() {
    
    
    if (_do_disp_y)
    {
        fftwf_destroy_plan(_disp_y_plan);
    }
    
    if (_do_normals)
    {
        fftwf_destroy_plan(_N_x_plan);
        fftwf_destroy_plan(_N_z_plan);
    }
    
    if (_do_chop)
    {
        fftwf_destroy_plan(_disp_x_plan);
        fftwf_destroy_plan(_disp_z_plan);
    }
    
    if (_do_jacobian)
    {
        fftwf_destroy_plan(_Jxx_plan);
        fftwf_destroy_plan(_Jzz_plan);
        fftwf_destroy_plan(_Jxz_plan);
    }

    
    
}


drw::OceanContext::OceanContext(Int32 m,Int32 n,float Lx,float Lz,Bool hf,Bool chop,Bool normals,Bool jacobian): _M(m),_N(n),_Lx(Lx),_Lz(Lz), _do_disp_y(hf),_do_normals(normals),_do_chop(chop),_do_jacobian(jacobian)
{

    //Lock lock(*this);
    _fft_in.resize(_M,1 + _N/2);      
    _htilda.resize(_M,1 + _N/2);
    
    assert(sizeof (*_fft_in.data()) == sizeof (fftwf_complex));
    
    if (hf)       alloc_disp_y();
    if (normals)  alloc_normal();
    if (chop)     alloc_chop();
    if (jacobian) alloc_jacobian();
    
}




void drw::OceanContext::alloc_disp_y()
{
    _disp_y.resize(_M,_N);
    _disp_y_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                         reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                         reinterpret_cast<my_float*>     (_disp_y.data()),
                                         FFTW_ESTIMATE); 
}

void drw::OceanContext::alloc_chop()
{
    _disp_x.resize(_M,_N);
    _disp_z.resize(_M,_N);
    
    _disp_x_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                         reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                         reinterpret_cast<my_float*>     (_disp_x.data()),
                                         FFTW_ESTIMATE);
    _disp_z_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                         reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                         reinterpret_cast<my_float*>     (_disp_z.data()),
                                         FFTW_ESTIMATE);
}

void drw::OceanContext::alloc_normal()
{
    _N_x.resize(_M,_N);
    _N_y.resize(_M,_N);
    _N_z.resize(_M,_N);
    
    _N_x_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                      reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                      reinterpret_cast<my_float*>     (_N_x.data()),
                                      FFTW_ESTIMATE);
    _N_z_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                      reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                      reinterpret_cast<my_float*>     (_N_z.data()),
                                      FFTW_ESTIMATE);
}
void drw::OceanContext::alloc_jacobian()
{
    _Jxx.resize(_M,_N);
    _Jzz.resize(_M,_N);
    _Jxz.resize(_M,_N);
    
    _Jxx_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                      reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                      reinterpret_cast<my_float*>     (_Jxx.data()),
                                      FFTW_ESTIMATE);
    _Jzz_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                      reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                      reinterpret_cast<my_float*>     (_Jzz.data()),
                                      FFTW_ESTIMATE);
    
    _Jxz_plan = fftwf_plan_dft_c2r_2d(_M,_N,
                                      reinterpret_cast<fftwf_complex*>(_fft_in.data()),
                                      reinterpret_cast<my_float*>     (_Jxz.data()),
                                      FFTW_ESTIMATE);
}



void drw::OceanContext::eval_uv(float u,float v)
{
    Int32 i0,i1,j0,j1;
    float frac_x,frac_z;
    
    // first wrap the texture so 0 <= (u,v) < 1
    u = fmod(u,1.0f);
    v = fmod(v,1.0f);
    
    if (u < 0) u += 1.0f;
    if (v < 0) v += 1.0f;
    
    float uu = u * _M;
    float vv = v * _N;
    
    i0 = (Int32)floor(uu);
    j0 = (Int32)floor(vv);
    
    i1 = (i0 + 1);
    j1 = (j0 + 1);
    
    frac_x = uu - i0;
    frac_z = vv - j0;
    
    i0 = i0 % _M;
    j0 = j0 % _N;
    
    i1 = i1 % _M;
    j1 = j1 % _N;
    
    
#define BILERP(m) (lerp(lerp(m(i0,j0),m(i1,j0),frac_x),lerp(m(i0,j1),m(i1,j1),frac_x),frac_z))
    {
        if (_do_disp_y)
        {
            disp[1] = BILERP(_disp_y);
        }
        if (_do_normals)
        {
            normal[0] = BILERP(_N_x);
            normal[1] = BILERP(_N_y);
            normal[2] = BILERP(_N_z);
        }
        if (_do_chop) 
        {
            disp[0] = BILERP(_disp_x);
            disp[2] = BILERP(_disp_z); 
        }
        else
        {
            disp[0] = 0.0;
            disp[2] = 0.0;
        }
        if (_do_jacobian)
        {
            compute_eigenstuff(BILERP(_Jxx),BILERP(_Jzz),BILERP(_Jxz));
        }                      
    }
#undef BILERP
}

// use catmullrom interpolation rather than linear
void drw::OceanContext::eval2_uv(float u,float v)
{
    Int32 i0,i1,i2,i3,j0,j1,j2,j3;
    float frac_x,frac_z;
    
    // first wrap the texture so 0 <= (u,v) < 1
    u = fmod(u,1.0f);
    v = fmod(v,1.0f);
    
    if (u < 0) u += 1.0f;
    if (v < 0) v += 1.0f;
    
    float uu = u * _M;
    float vv = v * _N;
    
    i1 = (Int32)floor(uu);
    j1 = (Int32)floor(vv);
    
    i2 = (i1 + 1);
    j2 = (j1 + 1);
    
    frac_x = uu - i1;
    frac_z = vv - j1;
    
    i1 = i1 % _M;
    j1 = j1 % _N;
    
    i2 = i2 % _M;
    j2 = j2 % _N;
    
    i0 = (i1-1);
    i3 = (i2+1);
    i0 = i0 <   0 ? i0 + _M : i0;
    i3 = i3 >= _M ? i3 - _M : i3;
    
    j0 = (j1-1);
    j3 = (j2+1);
    j0 = j0 <   0 ? j0 + _N : j0;
    j3 = j3 >= _N ? j3 - _N : j3;
    
#define INTERP(m) catrom(catrom(m(i0,j0),m(i1,j0),m(i2,j0),m(i3,j0),frac_x), \
catrom(m(i0,j1),m(i1,j1),m(i2,j1),m(i3,j1),frac_x), \
catrom(m(i0,j2),m(i1,j2),m(i2,j2),m(i3,j2),frac_x), \
catrom(m(i0,j3),m(i1,j3),m(i2,j3),m(i3,j3),frac_x), \
frac_z)
    
    {
        if (_do_disp_y)
        {
            disp[1] = INTERP(_disp_y) ;
        }
        if (_do_normals)
        {
            normal[0] = INTERP(_N_x);
            normal[1] = INTERP(_N_y);
            normal[2] = INTERP(_N_z);
        }
        if (_do_chop) 
        {
            disp[0] = INTERP(_disp_x);
            disp[2] = INTERP(_disp_z); 
        }
        else
        {
            disp[0] = 0.0;
            disp[2] = 0.0;
        }
        
        if (_do_jacobian)
        {
            compute_eigenstuff(INTERP(_Jxx),INTERP(_Jzz),INTERP(_Jxz));
        }                      
    }
#undef INTERP
    
}

inline void drw::OceanContext::compute_eigenstuff(const my_float& jxx,const my_float& jzz,const my_float& jxz)
{
    my_float a,b,qplus,qminus;
    a = jxx + jzz; 
    b = sqrt((jxx - jzz)*(jxx - jzz) + 4 * jxz * jxz);
    
    Jminus = 0.5*(a-b);
    Jplus  = 0.5*(a+b);
    
    qplus  = (Jplus  - jxx)/jxz;
    qminus = (Jminus - jxx)/jxz;
    
    a = sqrt(1 + qplus*qplus);
    b = sqrt(1 + qminus*qminus);
    
    Eplus[0] = 1.0/ a;
    Eplus[1] = 0.0;
    Eplus[2] = qplus/a;
    
    Eminus[0] = 1.0/b;
    Eminus[1] = 0.0;
    Eminus[2] = qminus/b;
}

void drw::OceanContext::eval_xz(float x,float z)
{
    assert(_Lx != 0 && _Lz  != 0);
    eval_uv(x/_Lx,z/_Lz);
}
void drw::OceanContext::eval2_xz(float x,float z)
{
    assert(_Lx != 0 && _Lz  != 0);
    eval2_uv(x/_Lx,z/_Lz);
}


// note that this doesn't wrap properly for i,j < 0, but its
// not really meant for that being just a way to get the raw data out
// to save in some image format.
void drw::OceanContext::eval_ij(Int32 i,Int32 j)
{
    i = abs(i) % _M;
    j = abs(j) % _N;
    
    disp[1] = _do_disp_y ? _disp_y(i,j) : 0.0f;
    
    if (_do_chop)
    {
        disp[0] = _disp_x(i,j);
        disp[2] = _disp_z(i,j);
    }
    else
    {
        disp[0] = 0.0f;
        disp[2] = 0.0f;
    }
    
    
    if (_do_normals)
    {
        normal[0] = _N_x(i,j);
        normal[1] = _N_y(i,j);
        normal[2] = _N_z(i,j);
    }
    if (_do_jacobian)
    {
        compute_eigenstuff(_Jxx(i,j),_Jzz(i,j),_Jxz(i,j));
    }                      
}


drw::Ocean::Ocean(Int32 M,Int32 N,
                   my_float dx,my_float dz,
                   my_float V,
                   my_float l,
                   my_float A,
                   my_float w,
                   my_float damp,
                   my_float alignment,
                   my_float depth,
                   Int32 seed)
: _M(M),_N(N),
_V(V),_l(l),_A(A),_w(w),
_damp_reflections(damp),
_wind_alignment(alignment),
_depth(depth),
_Lx(M*dx),_Lz(N*dz),
_wx(cos(w)),_wz(-sin(w)), // wave direction
_L(V*V / g)               // largest wave for a given velocity V

{     
   // Lock(*this);
    
    // size the arrays
    _k.resize(M,1 + N/2);
    _h0.resize(M,N);
    _h0_minus.resize(M,N);
    _kx.resize(_M);
    _kz.resize(_N);
    
    // make this robust in the face of erroneous usage
    if (_Lx == 0.0)
    {
        _Lx = 0.001;
        std::cerr << "warning: Ocean has been given a zero size computational domain\n";
    }
    if (_Lz == 0.0)
    {
        _Lz = 0.001;
        std::cerr << "warning: Ocean has been given a zero size computational domain\n";
    }
    
    
    // Calculate the frequency components, we do this in the order that the ifft routine 
    // requires its arguments, instead of translating the results from [-N/2,N/2) to [0,N/2).
    // The other examples I've seen don't do this, so I hope I'm not missing something, it
    // just seems easier this way.
    
    
    // This is the way described in the paper, where the
    // shift is corrected later ...
    // 
    //for (Int32 i = 0 ; i  < _M ; ++i)
    //{
    //    _kx(i) = 2.0f * pi * (i + -_M/2)/ _Lx;
    //}
    //for (Int32 j = 0 ; j < _N ; ++j)
    //{
    //    _kz(j) = 2.0f * pi * (j + -_N/2)/ _Lz;
    //}   
    
    // the +ve components and DC
    for (Int32 i = 0 ; i <= _M/2 ; ++i)
    { 
        _kx(i) = 2.0f * PI * i / _Lx;
    }
    // the -ve components
    for (Int32 i = _M-1,ii=0 ; i > _M/2 ; --i,++ii)
    {
        _kx(i) = -2.0f * PI * ii / _Lx;
    }
    
    // the +ve components and DC
    for (Int32 i = 0 ; i <= _N/2 ; ++i)
    { 
        _kz(i) = 2.0f * PI * i / _Lz;
    }
    // the -ve components
    for (Int32 i = _N-1,ii=0 ; i > _N/2 ; --i,++ii)
    {
        _kz(i) = -2.0f * PI * ii / _Lz;
    }
    
    // pre-calculate the k matrix
    for (Int32 i = 0 ; i  < _M ; ++i)
    {
        for (Int32 j  = 0 ; j  <= _N / 2 ; ++j) // note <= _N/2 here, see the fftw notes about complex->real fft storage
        {
            _k(i,j) = sqrt(_kx(i)*_kx(i) + _kz(j)*_kz(j) );
        }
    }
    

    // using the c4d random gaussian stuff instead of EXR gaussian lib :
    Random myrandNumber;
    myrandNumber.Init(seed);
    
    Imath::Rand32 rand(seed);
    
    // calculate htilda0 (see Tessendorf notes)
    // The h0_minus component is not strictly neccessary
    // but lets leave it for the time being for clarity.
    for (Int32 i = 0 ; i  < _M ; ++i)
    {
        for (Int32 j = 0 ; j  < _N ; ++j)
        {
            my_float r1 = Imath::gaussRand(rand);
            my_float r2 = Imath::gaussRand(rand);
            
           // my_float r1 = myrandNumber.GetG01();
          //  my_float r2 = myrandNumber.GetG01();
            

            
            _h0(i,j)       = complex_f(r1,r2) * float(sqrt(Ph( _kx(i), _kz(j)) / 2.0f));
            _h0_minus(i,j) = complex_f(r1,r2) * float(sqrt(Ph(-_kx(i),-_kz(j)) / 2.0f));
        }
    }
}  



drw::my_float drw::Ocean::Ph(my_float kx,my_float kz ) const
{
    my_float k2 = kx*kx + kz*kz;
    
    if (k2 == 0.0)
    {
        return 0.0; // no DC component
    }
    
    // damp out the waves going in the direction opposite the wind
    float tmp = (_wx * kx  + _wz * kz)/sqrt(k2);
    if (tmp < 0) 
    {
        tmp *= _damp_reflections;
    }
    
    return _A * exp( -1.0f / (k2*sqr(_L) ))  * exp(-k2 * sqr(_l)) * pow(fabs(tmp),_wind_alignment) / (k2*k2);
}

drw::my_float drw::Ocean::wavelength(my_float k)  const
{
    return 2.0f * PI / k;
}

drw::my_float drw::Ocean::omega(my_float k) const
{ 
    return sqrt(g*k * tanh(k*_depth));
}





void  drw::Ocean::update(float t, OceanContext& r, Bool do_heightfield, Bool do_chop, Bool do_normal, Bool do_jacobian, float scale, float chop_amount)
{
    
    // fftw is re-entrant for fft_execute calls ... so we shouldn't need the following line ...
    // Lock lock(*this);
    
    assert(r._M==_M && r._N==_N);
    
    // compute a new htilda
    for (Int32 i = 0 ; i  < _M ; ++i)
    {
        // note the <= _N/2 here, see the fftw doco about
        // the mechanics of the complex->real fft storage
        for (Int32 j  = 0 ; j  <= _N / 2 ; ++j)
        {
            r._htilda(i,j) = _h0(i,j) * exp(complex_f(0,omega(_k(i,j))*t))  + 
            conj(_h0_minus(i,j)) * exp(complex_f(0,-omega(_k(i,j))*t));
        }
    }
    r._fft_in = scale * r._htilda;
    
    if (do_heightfield && r._do_disp_y)
    {
        // y displacement
        fftwf_execute(r._disp_y_plan);
    }
    
    if (do_chop && r._do_chop)
    {
        // x displacement
        for (Int32 i = 0 ; i  < _M ; ++i)
        {   
            for (Int32 j  = 0 ; j  <= _N / 2 ; ++j)
            {     
                r._fft_in(i,j) = -scale * chop_amount * minus_i * 
                r._htilda(i,j) * (_k(i,j) == 0.0 ? complex_f(0,0) : _kx(i) / _k(i,j)) ;   
            }
        }
        fftwf_execute(r._disp_x_plan);
        
        // z displacement
        for (Int32 i = 0 ; i  < _M ; ++i)
        {   
            for (Int32 j  = 0 ; j  <= _N / 2 ; ++j) 
            {                    
                r._fft_in(i,j) = -scale * chop_amount * minus_i * 
                r._htilda(i,j) * (_k(i,j) == 0.0 ? complex_f(0,0) : _kz(j) / _k(i,j)) ;
            }
        } 
        fftwf_execute(r._disp_z_plan);
        
    }
    
    // fft normals
    if (do_normal && r._do_normals)
    {
        for (Int32 i = 0 ; i  < _M ; ++i)
        {
            for (Int32 j  = 0 ; j  <= _N / 2 ; ++j)
            {     
                r._fft_in(i,j) = - plus_i * r._htilda(i,j) * _kx(i)  ;   
            }
        }
        fftwf_execute(r._N_x_plan);
        
        for (Int32 i = 0 ; i  < _M ; ++i)
        {   
            for (Int32 j  = 0 ; j  <= _N / 2 ; ++j) 
            {                    
                r._fft_in(i,j) = - plus_i * r._htilda(i,j) * _kz(j) ;
            }
        } 
        fftwf_execute(r._N_z_plan);
        
        r._N_y = 1.0f/scale;// todo: fix this waste of memory
        
    }
    
    
    if (do_jacobian && r._do_jacobian)
    {
        
        // Jxx
        for (Int32 i = 0 ; i  < _M ; ++i)
        {
            for (Int32 j  = 0 ; j  <= _N / 2 ; ++j)
            {     
                r._fft_in(i,j) = -scale * chop_amount * r._htilda(i,j) *  
                (_k(i,j) == 0.0 ? complex_f(0,0) : _kx(i)*_kx(i) / _k(i,j));   
            }
        }
        fftwf_execute(r._Jxx_plan);
        r._Jxx += 1.0;
        
        // Jzz
        for (Int32 i = 0 ; i  < _M ; ++i)
        {
            for (Int32 j  = 0 ; j  <= _N / 2 ; ++j)
            {     
                r._fft_in(i,j) = -scale * chop_amount * r._htilda(i,j) * 
                (_k(i,j) == 0.0 ? complex_f(0,0) : _kz(j)*_kz(j) / _k(i,j));
            }
        }
        fftwf_execute(r._Jzz_plan);
        r._Jzz += 1.0;
        
        // Jxz
        for (Int32 i = 0 ; i  < _M ; ++i)
        {
            for (Int32 j  = 0 ; j  <= _N / 2 ; ++j)
            {     
                r._fft_in(i,j) = -scale *chop_amount * r._htilda(i,j) * 
                (_k(i,j) == 0.0 ? complex_f(0,0) : _kx(i)*_kz(j) / _k(i,j));  
            }
        }
        fftwf_execute(r._Jxz_plan);
        
        // note: from here we can derive the eigenvalues and
        // vectors at the evaluation stage saving memory ...
        
    }
}

float drw::Ocean::get_height_normalize_factor() 
{  
    OceanContext *r = new_context(true,false,false,false);
    update(0.0,*r,true,false,false,false,1.0,0.0);
    
    float res = 1.0;
    
    my_float max_h = std::numeric_limits<my_float>::min( );
    
    for (Int32 i = 0 ; i < r->_disp_y.rows() ; ++i)
    {
        for (Int32 j = 0 ; j  < r->_disp_y.cols()  ; ++j)
        {   
            max_h = Max(max_h,fabsf(r->_disp_y(i,j)));
        }
    }
    
    if (max_h == 0.0) max_h = 0.00001f; // just in case ...
    
    res = 1.0f / max_h;
    
    //delete r;
	DeleteMem(r);
    return res;
}




drw::OceanContext *drw::Ocean::new_context(Bool hf,Bool chop,Bool normals,Bool jacobian)
{
     return NewObjClear(drw::OceanContext,_M,_N,_Lx,_Lz,hf,chop,normals,jacobian);
}





