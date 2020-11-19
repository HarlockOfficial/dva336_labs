#include <cmath>
#include <iostream>
#include <cstdlib>
#include <omp.h>

#define NUM_THREADS 8

struct bstnode {
	bstnode(int key) : key(key) { left = right = nullptr; }
	~bstnode() { delete left; delete right; }
	int key;
	bstnode *left, *right;
};

int max(int a, int b) { return a>b ? a : b; }

//returns the height of the tree
int height(bstnode* root) {
	if(!root) return 0;
	int lefth,righth;
	lefth = height(root->left);
	righth = height(root->right);
	return max(lefth,righth)+1;
}

int height_parallel(bstnode* root){
	if(!root) return 0;

	int left_height, right_height;

	#pragma omp task shared(left_height)
 	left_height = height_parallel(root->left);

	#pragma omp task shared(right_height)
	right_height = height_parallel(root->right);

	#pragma omp taskwait
	//std::cout<<"left height: "<<left_height<<" right height: "<<right_height<<"\n";
	return max(left_height,right_height)+1;
}

//returns true if the tree is height-balanced; false otherwise
bool isbalanced(bstnode* root) {
	if(!root) return true;
	int lefth,righth;
	lefth = height(root->left);
	righth = height(root->right);
	return abs(lefth-righth)<2 && isbalanced(root->left) && isbalanced(root->right);
}

bool is_balanced_parallel(bstnode* root){
	if(!root) return true;
	
	int left_height, right_height;
	bool is_balanced_left, is_balanced_right;

	#pragma omp task shared(left_height)
	left_height=height_parallel(root->left);

	#pragma omp task shared(right_height)
	right_height=height_parallel(root->right);

	#pragma omp task shared(is_balanced_left)
	is_balanced_left = is_balanced_parallel(root->left);
	
	#pragma omp task shared(is_balanced_right)
	is_balanced_right = is_balanced_parallel(root->right);

	#pragma omp taskwait
	//std::cout<<"left height: "<<left_height<< ""
	//		" right height: "<<right_height<<" is balanced left: "<<is_balanced_left<< ""
	//		" is balanced right"<<is_balanced_right<<"\n\n";
	return abs(left_height-right_height)<2 && is_balanced_right && is_balanced_left;
}

//return a pointer to a balanced BST of which keys are in [lower,upper]
bstnode* buildbalanced(int lower, int upper) {
  if (upper<lower) return nullptr;
  int key = (upper-lower+1)/2 + lower;
  bstnode* root = new bstnode(key);
  root->left = buildbalanced(lower, key-1);
  root->right = buildbalanced(key+1, upper);
  return root;
}



#define power_of_two(exp) (1<<(exp))

int main()
{
	bstnode *root = buildbalanced(1, power_of_two(14)-1);
	bool response;
	double tp = omp_get_wtime();
	#pragma  omp  parallel num_threads(NUM_THREADS)
	#pragma  omp  single
	response = is_balanced_parallel(root);
	tp=omp_get_wtime()-tp;
	std::cout<<"is_balanced_parallel(root) = "<<(response?'Y':'N')<<"\n";

	double ts = omp_get_wtime();
	std::cout<<"isbalanced(root) = "<<(isbalanced(root)?'Y':'N')<<"\n";
	ts = omp_get_wtime()-ts;

	delete root;
	
	std::cout<<"Sequential Version elapsed time = "<<ts<<" seconds\n";
	std::cout<<"Parallel Version elapsed time = "<<tp<<" seconds ("<<ts/tp<<"x speedup)\n";

	return 0;
}