#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <assert.h>
#include <x86intrin.h>
#include <iostream>
#define alignment 16	//using _m128

// start of changed section
using namespace std;

//modificare 
struct point2D {
	float x, y;
};
// end of changed section

struct circle {
	point2D center;
	float radius;
};


// start of changed section
struct mesh2D {
	mesh2D (const int n) : n(n) {
		assert(n>0 && n%16==0);
		x = (float*)aligned_alloc(alignment, n*sizeof(float));
		y = (float*)aligned_alloc(alignment, n*sizeof(float));
	}
	~mesh2D() {
		free(x);
		free(y);
	}
	void set(const point2D p, int i) {
		assert(i>=0 && i<n); 
		x[i] = p.x;
		y[i] = p.y;
	}
	
	circle calc_enclosingdisc() {
		circle enclosingdisc;
		__m128 maxx, maxy, minx, miny;
		minx = _mm_load_ps(x);
		maxx = _mm_load_ps(x);
		miny = _mm_load_ps(y);
		maxy = _mm_load_ps(y);
		/*
			using min and max on arrays and shifting second array of one position every turn
			let me have at position 0 the smallest and biggest element of the array
		*/
		for(int i=4;i<n;i+=4){	
			minx = _mm_min_ps(minx, _mm_load_ps(x+i));
			miny = _mm_min_ps(miny, _mm_load_ps(y+i));
			maxx = _mm_max_ps(maxx, _mm_load_ps(x+i));
			maxy = _mm_max_ps(maxy, _mm_load_ps(x+i));
		}
		//calc center
		/*float maxx_scalar = horizontal_max_Vec(maxx);
		float maxy_scalar = horizontal_max_Vec(maxy);
		float minx_scalar = horizontal_min_Vec(minx);
		float miny_scalar = horizontal_min_Vec(miny);*/
		float maxx_scalar = sseHorizontalMax(maxx);
		float maxy_scalar = sseHorizontalMax(maxy);
		float minx_scalar = sseHorizontalMin(minx);
		float miny_scalar = sseHorizontalMin(miny);
		enclosingdisc.center.x = (maxx_scalar-minx_scalar)/2+minx_scalar;
		enclosingdisc.center.y = (maxy_scalar-miny_scalar)/2+miny_scalar;
		
		//calc radius
		float *maxsqdst=(float*)aligned_alloc(alignment, n*sizeof(float));
		for(int i=0; i<n; ++i) {
			maxsqdst[i] = (x[i]-enclosingdisc.center.x)*(x[i]-enclosingdisc.center.x)+
				(y[i]-enclosingdisc.center.y)*(y[i]-enclosingdisc.center.y);
		}

		__m128 max_rad = _mm_load_ps(maxsqdst);
		for(int i = 4;i<n;i+=4){
			max_rad = _mm_max_ps(max_rad, _mm_load_ps(maxsqdst+i));
		}
	
		enclosingdisc.radius = sqrtf(sseHorizontalMax(max_rad));
		//return enclosing disc
		return enclosingdisc;
	}
private:
	const int n = 0;
	float *x, *y;

	//https://stackoverflow.com/questions/46125647/finding-maximum-float-in-sse-vector-m128
	//slower (not used)
	/*float horizontal_max_Vec(__m128 x) {
		__m128 max1 = _mm_shuffle_ps(x, x, _MM_SHUFFLE(0,0,3,2));
		__m128 max2 = _mm_max_ps(x, max1);
		__m128 max3 = _mm_shuffle_ps(max2, max2, _MM_SHUFFLE(0,0,0,1));
		__m128 max4 = _mm_max_ps(max2, max3);
		return _mm_cvtss_f32(max4);
	}
	//edited for min
	float horizontal_min_Vec(__m128 x) {
		__m128 min1 = _mm_shuffle_ps(x, x, _MM_SHUFFLE(0,0,3,2));
		__m128 min2 = _mm_min_ps(x, min1);
		__m128 min3 = _mm_shuffle_ps(min2, min2, _MM_SHUFFLE(0,0,0,1));
		__m128 min4 = _mm_min_ps(min2, min3);
		return _mm_cvtss_f32(min4);
	}*/

	//http://www.awsom3d.com/2014/06/sse-horizontal-minimum-and-maximum.html
	static inline float sseHorizontalMin(const __m128 &p)  {   
		__m128 data = p;
		__m128 low = _mm_movehl_ps(data, data);
		__m128 low_accum = _mm_min_ps(low, data);
		__m128 elem1 = _mm_shuffle_ps(low_accum,   
						low_accum,   
						_MM_SHUFFLE(1,1,1,1));
		__m128 accum = _mm_min_ss(low_accum, elem1);   
		return _mm_cvtss_f32(accum);   
	}  
	static inline float sseHorizontalMax(const __m128 &p)  {
		__m128 data = p;
		__m128 high = _mm_movehl_ps(data, data);
		__m128 high_accum = _mm_max_ps(high, data);
		__m128 elem1 = _mm_shuffle_ps(high_accum,   
						high_accum,   
						_MM_SHUFFLE(1,1,1,1));
		__m128 accum = _mm_max_ss(high_accum, elem1);   
		return _mm_cvtss_f32(accum);   
	} 
};

inline point2D randompoint() {
	point2D p;
	p.x = 2.0f*rand()/(RAND_MAX+1.0f);
	p.y = 2.0f*rand()/(RAND_MAX+1.0f);
	return p;
}
// end of changed section

int main(int argc, char *argv[]) {
	srand(12345);
	const int n = atoi(argv[1]);
	mesh2D m(n);
	for(int i=0; i<n; ++i)
		m.set(randompoint(), i);
	clock_t t = clock();
	circle res = m.calc_enclosingdisc();
	t = clock()-t;
	printf("center = (%g,%g)\nradius = %.10f\nelapsed time = %.3f sec\n", res.center.x, res.center.y, res.radius, (double)t/CLOCKS_PER_SEC);	
	return 0;
}
