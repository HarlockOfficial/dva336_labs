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

struct point2D {
	float *x, *y;
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
		data.x = (float*)aligned_alloc(alignment, n*sizeof(float));
		data.y = (float*)aligned_alloc(alignment, n*sizeof(float));
	}
	~mesh2D() {
		free(data.x);
		free(data.y);
	}
	void set(const point2D p, int i) {
		assert(i>=0 && i<n); 
		data.x[i] = p.x[0];
		data.y[i] = p.y[0];
	}
	
	circle calc_enclosingdisc() {
		circle enclosingdisc;
		__m128 maxx, maxy, minx, miny;
		minx = _mm_load_ps(data.x);
		maxx = _mm_load_ps(data.x);
		miny = _mm_load_ps(data.y);
		maxy = _mm_load_ps(data.y);
		/*
			using min and max on arrays and shifting second array of one position every turn
			let me have at position 0 the smallest and biggest element of the array
		*/
		for(int i=4;i<n;i+=4){	
			minx = _mm_min_ps(minx, _mm_load_ps(data.x+i));
			miny = _mm_min_ps(miny, _mm_load_ps(data.y+i));
			maxx = _mm_max_ps(maxx, _mm_load_ps(data.x+i));
			maxy = _mm_max_ps(maxy, _mm_load_ps(data.x+i));
		}
		//calc center
		enclosingdisc.center.x = (float*)aligned_alloc(alignment, sizeof(float));
		enclosingdisc.center.x[0] = (maxx[0]-minx[0])/2+minx[0];
		enclosingdisc.center.y = (float*)aligned_alloc(alignment, sizeof(float));
		enclosingdisc.center.y[0] = (maxy[0]-miny[0])/2+miny[0];
		
		//calc radius
		float maxsqdst[n];
		for(int i=0; i<n; ++i) {
			maxsqdst[i] = (data.x[i]-enclosingdisc.center.x[0])*(data.x[i]-enclosingdisc.center.x[0])+
				(data.y[i]-enclosingdisc.center.y[0])*(data.y[i]-enclosingdisc.center.y[0]);
		}
		__m128 min_rad = _mm_load_ps(maxsqdst);
		for(int i = 4;i<n;i+=4){
			min_rad = _mm_max_ps(min_rad, _mm_load_ps(maxsqdst+i));
		}
		enclosingdisc.radius = sqrtf(min_rad[0]);
		//return enclosing disc
		return enclosingdisc;
	}
private:
	const int n = 0;
	point2D data;
};

inline point2D randompoint() {
	point2D p;
	p.x = new float[1];
	p.y = new float[1];
	p.x[0] = 2.0f*rand()/(RAND_MAX+1.0f);
	p.y[0] = 2.0f*rand()/(RAND_MAX+1.0f);
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
	printf("center = (%g,%g)\nradius = %.10f\nelapsed time = %.3f sec\n", res.center.x[0], res.center.y[0], res.radius, (double)t/CLOCKS_PER_SEC);	
	return 0;
}
