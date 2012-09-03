#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>

long timer(int reset)
{
	static long start = 0;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	/* return timediff */
	if (!reset)
	{
		long stop = ((long) tv.tv_sec) * 1000000 + tv.tv_usec ;
		return (stop - start);
	}

	/* reset timer */
	start = ((long) tv.tv_sec) *1000000  + tv.tv_usec  ;

	return 0;
}

int main(int argc, char** argv)
{
   int i = 0;
   int res;
   int len;
   char result[BUFSIZ];
   char err_buf[BUFSIZ];
   char *src = argv[1];   /* 源由命令行参数指定 */
   char *pstart = NULL;
   char *pcur = NULL; 

   char params[3][32];
   char *a,*b,*c;

   printf("[%c]", params[0][0]);
   
   

    //100x100gb_r120_q70.jpg
    //wKgUy07qtkySUKdAAAG76b0tf7E477_000_000.jpeg
    //                32            _r[2-3]_q[2-3].[3-4]
    timer(1);
    int index;
    for(i=0;i<10000;i++) {
    	pcur = src;
    	pstart  = NULL;
    	index = 0;
    	while (*pcur != '\0') {
    		if (*pcur == '_' || *pcur == '.')	{
    			if( pstart != NULL ) {
    				memcpy(params[index], pstart, pcur - pstart);
    				index++;
    			}
    			pstart = pcur;
    			pstart++;
    		}
    		pcur++;
    	}
	   	//break;
		
	}
	a = (params[0]);
	b = (params[1]);
	c = (params[2]);

	size_t tt = 0;
	pcur = b;
	pcur++;	
	tt = (size_t) atol(pcur);

	printf("params:[%s],len:%d\n", pcur, tt);
	printf("params:[%s]\n", b);
	printf("params:[%s]\n", c);
   	printf("src:[%s]\n", src);

	printf("time:[%d]\n", timer(0));


	return 0;
	

   //const char* pattern = "\\<[^,;]+\\>";
   const char* pattern = "[^_]*_([^_]*)";
   //const char* pattern = "([0-9]+)x([0-9]+)\\w\\w_r([0-9]+)_q([0-9]+)\\.[\\w]{3}";
   regex_t preg;
   
   regmatch_t pmatch[10];

   if( (res = regcomp(&preg, pattern, REG_EXTENDED)) != 0)
   {
      regerror(res, &preg, err_buf, BUFSIZ);
      printf("regcomp: %s\n", err_buf);
      exit(res);
   }

   res = regexec(&preg, src, 10, pmatch, REG_NOTBOL);
   //~ res = regexec(&preg, src, 10, pmatch, 0);
   //~ res = regexec(&preg, src, 10, pmatch, REG_NOTEOL);
   if(res == REG_NOMATCH)
   {
      printf("NO match\n");
      exit(0);
   }
   for (i = 0; pmatch[i].rm_so != -1; i++)
   {
      len = pmatch[i].rm_eo - pmatch[i].rm_so;
      memcpy(result, src + pmatch[i].rm_so, len);
      result[len] = 0;
      printf("num %d: '%s'\n", i, result);
   }
   regfree(&preg);
   return 0;
}
