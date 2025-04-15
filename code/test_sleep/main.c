#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	
	printf("Before sleep \n");
	sleep(1);
	printf("After sleep\n");
	return 0;
}
