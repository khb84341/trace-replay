#include <iostream>
#include <cstdio>
#include <cstring>
using namespace std;

#define MAX_PATH 100

int main(void)
{
	char tmp[10] = "asdfasdf";
	char *buf = tmp;

	cout << "buf: " << buf << ", strlen: " << strlen(buf) << endl;
}
