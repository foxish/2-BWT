/*  FM-Index - Text Index
 *  Copyright (C) 2011  Matthias Petri
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "FM.h"

static void
print_usage(const char *program)
{
    fprintf(stderr, "USAGE: %s -i <index> <qrys>\n", program);
	fprintf(stderr, "  qrys : file containing queries\n");
	fprintf(stderr, "  index : index file\n");
	fprintf(stderr, "  -v verbose output\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "EXAMPLE: %s -i alice29.fm alice29.qrys\n",program);
    fprintf(stderr, "\n");
    return;
}

/*
 * 
 */
int main(int argc, char** argv) {
    int32_t opt,nqrys,maxqry,i,errors = 0;
    char* idxname;char* qryname;
    FILE* f;
    FM* FMIdx;
	uint8_t** queries;
	char buf[4096];
	uint32_t start,stop,cnt;
    
    /* parse command line parameter */
    if (argc <= 3) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
	opt = -1;
    idxname = qryname = NULL;
    while ((opt = getopt(argc, argv, "vhi:")) != -1) {
        switch (opt) {
			case 'i':
				idxname = optarg;
				break;
			case 'v':
				FM::verbose = 1;
				break;
            case 'h':
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }
	/* read filenames */
	if(optind < argc) { 
		qryname = argv[optind];
	}
	/* read error count */
  if(optind+1 < argc) { 
    errors = atoi(argv[optind+1]);
    if (errors < 0 || errors > 2) {
      perror("unsupported match approximation level given");
      exit(EXIT_FAILURE);
    }
  }
	
	if(qryname==NULL) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
		
	/* load index */
	FMIdx = FM::load(idxname);
	if(!FMIdx) {
		perror("error loading index from file");
		exit(EXIT_FAILURE);
	}
	
	/* read queries */
	FM::info("reading %queries");
	f = safe_fopen(qryname,"r");
	maxqry = REALLOC_INCREMENT;
	queries = (uint8_t**) safe_malloc(REALLOC_INCREMENT * sizeof(uint8_t*));
	nqrys = 0;
	while( fscanf(f,"%s\n",buf) == 1 ) {
		queries[nqrys] = (uint8_t*) safe_strdup(buf);
		if(nqrys == maxqry-1) {
			queries = (uint8_t**) safe_realloc(queries,
											(maxqry*2)*sizeof(uint8_t*));
			maxqry *= 2;
		}
		nqrys++;
	}
	fclose(f);
	FM::info("read %d queries",nqrys);
	
	start = gettime();
	for(i=0;i<nqrys;i++) {
    if (errors == 0) {
      // search for exact pattern
      cnt = FMIdx->count(queries[i],strlen((char*)queries[i]));
    } else {
      std::vector<SA_intervals> matching_intervals;
      switch(errors) {
        case 1:
          matching_intervals = FMIdx->locate1(queries[i],strlen((char*)queries[i]));
          break;
        case 2:
          matching_intervals = FMIdx->locate2(queries[i],strlen((char*)queries[i]));
          break;
      }
      cnt = 0;
      for (size_t it = 0; it < matching_intervals.size(); ++it) {
        SA_intervals ivls = matching_intervals[it];
        cnt += ivls.ivl.ep - ivls.ivl.sp + 1;
      }
    }

    fprintf(stdout,"%s : %d\n",queries[i],cnt);
	}
	stop = gettime();
	FM::info("finished processing queries: %.3f sec",((float)(stop-start))/1000000);
	
	/* clean up */
	for(i=0;i<nqrys;i++) free(queries[i]);
	free(queries);
	delete FMIdx;
	/* T already deleted in FMIdx */
    
    return (EXIT_SUCCESS);
}

