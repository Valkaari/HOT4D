//
//  Ocean.h
//  waves
//
//  Created by Manuel MAGALHAES on 10/02/12.
//  Copyright 2012 Valkaari. All rights reserved.
//

#ifndef _ocean_h
#define _ocean_h


#include <complex>
#include <vector>
#include "c4d.h"
#include "typedef.h"



// The "fastest fft in the west" (http://www.fftw.org/)
#include "fftw3.h"

// we use blitz for n-dimensional arrays (http://www.oonumerics.org/blitz/)
//#define BZ_THREADSAFE
#include <blitz/array.h>




namespace drw {
    typedef float                             my_float;
    typedef std::complex<my_float>            complex_f;
    typedef blitz::Array<my_float,1>          vector_f;
    typedef blitz::Array<my_float,2>          matrix_f;
    typedef blitz::Array<complex_f,2>         matrix_c;
    
    // functions //
    
    template <typename T> static inline T sqr(T x) {return x*x; }
    template <typename T> static inline T lerp(T a,T b,T f) { return a + (b-a)*f; }
    template <typename T> static inline T catrom(T p0,T p1,T p2,T p3,T f) 
    { 
        return 0.5 *((2 * p1) +
                     (-p0 + p2) * f +
                     (2*p0 - 5*p1 + 4*p2 - p3) * f*f +
                     (-p0 + 3*p1- 3*p2 + p3) * f*f*f);
    }
    
    
    
    
    
    const float g = 9.81f;
    const complex_f minus_i(0,-1);
    const complex_f plus_i(0,1);
    
    ////////////////////////////////////
    // 
    ////////////////////////////////////
    
    class OceanContext
    
    {	
        public :
        int             _M , _N;
        
        my_float           _Lx, _Lz;
        
        matrix_c        _fft_in,  _htilda;
        
        //FFTW plans;
        fftwf_plan      _disp_y_plan, _disp_x_plan, _disp_z_plan;
        fftwf_plan      _N_x_plan , _N_z_plan;
        fftwf_plan      _Jxx_plan, _Jxz_plan, _Jzz_plan;
        
        matrix_f        _disp_x, _disp_y , _disp_z;
        matrix_f        _N_x , _N_y , _N_z;
        
        
        matrix_f        _Jxx , _Jzz, _Jxz ;
        bool            _do_disp_y , _do_normals, _do_chop , _do_jacobian;
        float           disp[3], normal[3] , Jminus, Jplus,Eminus[3],Eplus[3] ;
        
        
        
        void            alloc_disp_y();
        void            alloc_chop();
        void            alloc_normal();
        void            alloc_jacobian();
        void            eval_uv(float u,float v);
        void            eval2_uv(float u,float v);
        inline void     compute_eigenstuff(const my_float& jxx,const my_float& jzz,const my_float& jxz);
        void            eval_xz(float x,float z);
        void            eval2_xz(float x,float z);
        void            eval_ij(int i,int j);
        
        ~OceanContext(void);
        
        
    private:
        OceanContext(int m,int n,float Lx,float Lz,bool hf,bool chop,bool normals,bool jacobian);
        friend class Ocean;
        
    };
    
    class Ocean
    {
        public :
        
        Ocean(int M,int N,
              my_float dx,my_float dz,
              my_float V,
              my_float l,
              my_float A,
              my_float w,
              my_float damp,
              my_float alignment,
              my_float depth,
              int seed);
        
        virtual ~Ocean() 
        {
            
        }
        OceanContext *new_context(bool hf,bool chop,bool normals,bool jacobian);
        
        
        my_float            Ph(my_float kx,my_float kz ) const;
        
        
        void                update(float t, OceanContext& r, bool do_heightfield, bool do_chop, bool do_normal, bool do_jacobian, float scale, float chop_amount);
        my_float            wavelength(my_float k)  const;
        my_float            omega(my_float k) const;
        float               get_height_normalize_factor();
        
        
    protected:
        
        friend class OceanContext;
        
        int _M;
        int _N;
        
        my_float _V;
        my_float _l;
        my_float _w;
        my_float _A;
        my_float _damp_reflections;
        my_float _wind_alignment;
        my_float _depth;
        
        my_float _wx;
        my_float _wz;
        
        my_float _L;
        my_float _Lx;
        my_float _Lz;
        vector_f _kx;
        vector_f _kz;
        
        matrix_c _h0;
        matrix_c _h0_minus;
        matrix_f  _k;

        
    };
    
    
    
}


#endif // ocean_h