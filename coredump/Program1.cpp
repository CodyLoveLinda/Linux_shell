// Segmentation Error Demonstration
// Author - Rohan Prasad
#include <iostream>
using namespace std;

int main()
{
	int* p = NULL;

	// This lines cause the segmentation
	// error because of accessing the
	// unknown memory location.
	*p = 1;

	cout << *p;
	return 0;
}

