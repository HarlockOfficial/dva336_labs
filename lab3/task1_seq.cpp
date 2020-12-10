#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <assert.h>

struct point2D {
	float x, y;
};

struct circle {
	point2D center;
	float radius;
};

struct mesh2D {
	mesh2D (const int n) : n(n) {
		assert(n>0 && n%16==0);
		data = (point2D*)malloc(n*sizeof(point2D));
	}
	~mesh2D() {
		free(data);
	}
	void set(const point2D p, int i) {
		assert(i>=0 && i<n); 
		data[i] = p; 
	}
	circle calc_enclosingdisc() {
		circle enclosingdisc;
		//calc center
		point2D min = data[0];
		point2D max = data[0];
		for(int i=1; i<n; ++i) 
			min.x = data[i].x<min.x ? data[i].x : min.x,
			max.x = data[i].x>max.x ? data[i].x : max.x,
			min.y = data[i].y<min.y ? data[i].y : min.y,
			max.y = data[i].y>max.y ? data[i].y : max.y;
		enclosingdisc.center.x = (max.x-min.x)/2+min.x,
		enclosingdisc.center.y = (max.y-min.y)/2+min.y;
		//calc radius
		float tmp, maxsqdst = 0.0f;
		for(int i=0; i<n; ++i) {
			tmp = (data[i].x-enclosingdisc.center.x)*(data[i].x-enclosingdisc.center.x)+(data[i].y-enclosingdisc.center.y)*(data[i].y-enclosingdisc.center.y);
			maxsqdst = maxsqdst>tmp ? maxsqdst : tmp;
		}
		enclosingdisc.radius = sqrtf(maxsqdst);
		//return enclosing disc
		return enclosingdisc;
	}
private:
	const int n = 0;
	point2D* data = nullptr;
};

inline point2D randompoint() {
	return {2.0f*rand()/(RAND_MAX+1.0f),2.0f*rand()/(RAND_MAX+1.0f)};
}

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
