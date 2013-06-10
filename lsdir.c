#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char **argv)
{
	DIR* dir = NULL;
	struct dirent *entry;
    int b;

	dir=opendir(argv[1]);

	if(dir==NULL) {
		perror("Error in opening directory !");
		return 1;
	}

	while(1) {
	  entry = readdir(dir);
	  if(entry == NULL) {
        if (errno) {
    	  	perror("Error in reading directory !");
	      	return 1;
        }
        return 0;
	  }

      b = 0;
      while (entry->d_name[b])
          printf("%02hhx ", (int)entry->d_name[b++]);
      printf("\n");
    }
	
	return 0;
}

