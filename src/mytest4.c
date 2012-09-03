/* gmtime example */
#include <stdio.h>
#include <time.h>

#define MST (-7)
#define UTC (0)
#define CCT (+8)

int main ()
{
	time_t rawtime;
	struct tm * ptm;

	time ( &rawtime );

	rawtime += 86400;
	ptm = gmtime ( &rawtime );

	/*

	puts ("Current time around the World:");
	printf ("Phoenix, AZ (U.S.) :  %2d:%02d\n", (ptm->tm_hour+MST)%24, ptm->tm_min);
	printf ("Reykjavik (Iceland) : %2d:%02d\n", (ptm->tm_hour+UTC)%24, ptm->tm_min);
	printf ("Beijing (China) :     %2d:%02d\n", (ptm->tm_hour+CCT)%24, ptm->tm_min);
	*/
	char p1[64];
	strftime(p1, 64 , "%a, %d %b %Y %H:%M:%S GMT\n", ptm);
	printf("time:%s,%d\n", p1, strlen(p1));
  
  	return 0;
}

