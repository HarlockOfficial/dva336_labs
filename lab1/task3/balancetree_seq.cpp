#include <cmath>
#include <iostream>
#include <cstdlib>

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

//returns true if the tree is height-balanced; false otherwise
bool isbalanced(bstnode* root) {
	if(!root) return true;
	int lefth,righth;
	lefth = height(root->left);
	righth = height(root->right);
	return abs(lefth-righth)<2 && isbalanced(root->left) && isbalanced(root->right);
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
	std::cout<<"isbalanced(root) = "<<(isbalanced(root)?'Y':'N')<<"\n";
	delete root;
	return 0;
}