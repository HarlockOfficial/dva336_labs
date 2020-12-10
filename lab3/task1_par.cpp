#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <assert.h>
#include <x86intrin.h>

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
		data.x = (float*)aligned_alloc(alignment, n*sizeof(alignment));
		data.y = (float*)aligned_alloc(alignment, n*sizeof(alignment));
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
		maxx = minx = _mm_load_ps(data.x);
		maxy = miny = _mm_load_ps(data.y);
		
		/*
			using min and max on arrays and shifting second array of one position every turn
			let me have at position 0 the smallest and biggest element of the array
		*/
		for(int i = 4;i<n;i+=4){
			minx = _mm_min_ps(minx, _mm_load_ps((float *)(&minx+i)));
			miny = _mm_min_ps(miny, _mm_load_ps((float *)(&miny+i)));
			maxx = _mm_max_ps(maxx, _mm_load_ps((float *)(&maxx+i)));
			maxy = _mm_max_ps(maxy, _mm_load_ps((float *)(&maxy+i)));
		}

		//calc center
		enclosingdisc.center.x = new float[1]{(maxx[0]-minx[0])/2+minx[0]};
		enclosingdisc.center.y = new float[1]{(maxy[0]-miny[0])/2+miny[0]};

		//calc radius
		__m128 maxsqdst = _mm_setzero_ps();
		for(int i=0; i<n; ++i) {
			maxsqdst[i] = (data.x[i]-enclosingdisc.center.x[0])*(data.x[i]-enclosingdisc.center.x[0])+
				(data.y[i]-enclosingdisc.center.y[0])*(data.y[i]-enclosingdisc.center.y[0]);
		}
		for(int i = 4;i<n;i+=4){
			maxsqdst = _mm_max_ps(maxsqdst, _mm_load_ps((float *)(&maxsqdst+i)));
		}
		enclosingdisc.radius = sqrtf(maxsqdst[0]);
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
	printf("center = (%g,%g)\nradius = %.10f\nelapsed time = %.3f sec\n", res.center.x, res.center.y, res.radius, (double)t/CLOCKS_PER_SEC);	
	return 0;
}
