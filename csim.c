// Matthew LeMay - mlemay
// Jonathan Chang - jchang
// CS 2100 - B01 17
// Cachelab
#include "cachelab.h"
#include "getopt.h"
#include "ctype.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "math.h"

//
void printHelp();

typedef struct {
  char operation;
  unsigned long long int tag;
  unsigned int offset;

} trace_line_contents;

typedef struct {
  unsigned int v;
  unsigned long long int tag;
  unsigned int offset;
  unsigned long long int age;
} line;

unsigned int long long global_i;
/**
 * @brief Prints out helpful information about the program.
 *
 * This function is meant to give the end user an idea of how the program should
 * be used. It shows all the flags as well as their meanijng and some useful
 * examples.
 */
void printHelp() {
  printf("%s\n", "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>");
  printf("%s\n", "Options:");
  printf("%s\n", "  -h         Print this help message.");
  printf("%s\n", "  -v         Optional verbose flag.");
  printf("%s\n", "  -s <num>   Number of set index bits.");
  printf("%s\n", "  -E <num>   Number of lines per set.");
  printf("%s\n", "  -b <num>   Number of block offset bits.");
  printf("%s\n", "  -t <file>  Trace file.");
  printf("%s\n", " ");
  printf("%s\n", "Examples:");
  printf("%s\n", "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace");
  printf("%s\n", "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace");
}

/**
 * @brief Load operation on cache
 *
 * Calculates the set index for the address and then attempts to load the tag
 * from the set, by checkingto make sure the line is valid and tags equal
 *
 * @param line_command the given trace command.
 * @param cache the array of cache
 * @param s reference numer to number of sets
 * @param E number of lines per set
 * @param b reference number to number of bytes in line
 * @return 1 if load successful, 0 is load fail (unable to find tag in set)
 */
unsigned int load(trace_line_contents line_command, line **cache, unsigned int s, unsigned int E, unsigned int b) {
  unsigned int j;
  unsigned int tag_size = (64 - s - b);
  unsigned long long int tag_actual = line_command.tag >> (s + b);
  unsigned long long int temp = line_command.tag << tag_size;
  unsigned long long int i = temp >> (64 - s);
  global_i = i;

  for(j = 0; j < E; j++) {
    printf("| I:%llx V:%d Cache_Tage:%llx, Command_Tag:%llx ", i, cache[i][j].v, cache[i][j].tag, tag_actual);
    if(cache[i][j].v && (cache[i][j].tag == tag_actual)) {
      cache[i][j].age = 0;
      return 1;
    }
  }
  return 0;
}

/**
 * @brief Store operation on cache
 *
 * Calculates the set index for the address and then attempts to store the tag
 * in the set, by checking to make sure a line is empty
 *
 * @param line_command the given trace command.
 * @param cache the array of cache
 * @param s reference numer to number of sets
 * @param E number of lines per set
 * @param b reference number to number of bytes in line
 * @return 2, if tag already exists, 1 if store successful, 0 is store fail (set is full)
 */
unsigned int store(trace_line_contents line_command, line **cache, unsigned int s, unsigned int E, unsigned int b) {
  unsigned int j;
  unsigned int tag_size = (64 - s - b);
  unsigned long long int tag_actual = line_command.tag >> (s + b);
  unsigned long long int temp = line_command.tag << tag_size;
  unsigned long long int i = temp >> (64 - s);
  global_i = i;

  for(j = 0; j < E; j++) {
    if (!cache[i][j].v) {
      cache[i][j].v = 1;
      cache[i][j].tag = tag_actual;
      cache[i][j].offset = line_command.offset - 1;
      cache[i][j].age = 0;
      return 1;
    }
    if(cache[i][j].v && tag_actual == cache[i][j].tag) {
      cache[i][j].age = 0;
      return 2;
    }
  }
  return 0;
}

/**
 * @brief Eviction operation on cache
 *
 * Finds the oldest line on a set and then removes it
 *
 * @param line_command the given trace command.
 * @param cache the array of cache
 * @param s reference numer to number of sets
 * @param E number of lines per set
 * @param b reference number to number of bytes in line
 * @return 1 if eviction successful
 */
unsigned int eviction_notice(trace_line_contents line_command, line **cache, unsigned int s, unsigned int E, unsigned int b) {
  unsigned long long int j, maxAge, max_j;
  //unsigned int tag_size = (64 - s - b);
  //unsigned long long int temp =  line_command.tag << tag_size;
  //unsigned long long int i = temp >> (64 - s);
  unsigned long long int i = global_i;
  maxAge = 0;
  max_j = 0;

  for(j = 0; j < E; j++) {
    printf("%llx ", maxAge);
    printf("%llx ", i);
    printf("%llx ", j);
    printf("%d ", cache[i][j].v);
    printf("%llx", cache[i][j].age);
    if(cache[i][j].v && maxAge < cache[i][j].age) {
      maxAge = cache[i][j].age;
      max_j = j;
    }
  }
  cache[i][max_j].v = 0;
  cache[i][max_j].tag = 0;
  return 1;
}

int main(int argc, char **argv) {
  /////////////////
  /// VARIABLES ///
  /////////////////

  // Filename of the valgrind trace to replay.
  char *file_name;
  // Reference number to number of sets
  unsigned int s;
  // Number of sets
  unsigned int S;
  // Reference number to number of bytes per line
  unsigned int b;
  // Number of lines per set
  unsigned int E;
  // If 1, prints out helpful data
  unsigned int verbose_flag = 0;
  // If 1, prints usage information
  unsigned int help_flag = 0;
  // Keep track of number of hits
  unsigned int hit_count = 0;
  // Keep track of number of misses
  unsigned int miss_count = 0;
  // Keep track of number of evictions
  unsigned int eviction_count = 0;
  // Read command line arguments
  char c;

  // Read command line arguments and set the corresponding flag or variable
  // for each flag.
  while ((c = getopt (argc, argv, "s:E:b:t:hv")) != -1) {
    switch (c) {
      case 's':
        s = atoi(optarg);
        S = pow(2, s);
        break;
      case 'E':
        E = atoi(optarg);
        break;
      case 'b':
        b = atoi(optarg);
        break;
      case 't':
        file_name = optarg;
        break;
      case 'h':
        help_flag = 1;
        break;
      case 'v':
        verbose_flag = 1;
        break;
      default:
        abort();
      }
    }

    // If the user requests help, only print out the help message
    // don't attempt run the program
    if (help_flag) {
      printHelp();
      return 0;
    }

    // Open the valgrind output file
    FILE *file_contents = fopen(file_name, "r");

    // Make sure that the file has actually been opened, if not return with
    // an error.
    if (file_contents == 0) {
      perror("The file cannot be opened!\n");
      exit(-1);
    }

    line **cache = (line **)malloc(S * sizeof(line*));
    unsigned int i, j;
    for (i = 0; i < S; i++) {
      cache[i] = (line *)malloc(E * sizeof(line));
      for (j = 0; j < E; j++) {
        cache[i][j].v = 0;
        cache[i][j].tag = 0;
        cache[i][j].offset = 0;
        cache[i][j].age = 0;
      }
    }


    // Go through each line in the trace file
    trace_line_contents current_line_command;
    while (fscanf(file_contents, "%c %llx,%d\n", &current_line_command.operation,
           &current_line_command.tag, &current_line_command.offset) != EOF) {
      printf("WTF ");
      if(verbose_flag) {
          printf("%c %llx,%d ", current_line_command.operation, current_line_command.tag, current_line_command.offset);
      }
      printf("start command ");
      // Depending on given command do different operation
      if (current_line_command.operation == 'S') {
        // If current command is store
        unsigned int store_result = store(current_line_command, cache, s, E, b);
        if(store_result == 1) {
          // If store replaced empty line, count miss
          if(verbose_flag) {
            //printf("| store miss ");
            printf("miss ");
          }
          miss_count++;
        } else if(store_result == 2) {
          // If was already storing, count hit
          if(verbose_flag) {
            //printf("| store hit ");
            printf("hit ");
          }
          hit_count++;
        }else {
          // If unable to find empty line, evict and store, count miss count eviction
          if(verbose_flag) {
            //printf("| store eviction ");
            printf("miss ");
            printf("eviction ");
          }
          eviction_notice(current_line_command, cache, s, E, b);
          store(current_line_command, cache, s, E, b);
          eviction_count++;
          miss_count++;
        }

      // If current command in store
      } else if (current_line_command.operation == 'L') {
        if (load(current_line_command, cache, s, E, b)) {
          // If able to load, count hit
          if(verbose_flag) {
            //printf("| load hit ");
            printf("hit ");
          }
          hit_count++;
        } else {
          // If unable to load, count miss
          if(verbose_flag) {
            //printf("| load store miss ");
            printf("miss ");
          }
          if(store(current_line_command, cache, s, E, b)) {
            // If able to store
          } else {
            // If unable to store, evict count eviction
            if(verbose_flag) {
              //printf("| load store eviction ");
              printf("eviction ");
            }
            eviction_notice(current_line_command, cache, s, E, b);
            store(current_line_command, cache, s, E, b);
            eviction_count++;
          }
          // If load fails, count miss
          miss_count++;
        }

      // If current command is modify
      } else if (current_line_command.operation == 'M') {
        if (load(current_line_command, cache, s, E, b)) {
          // If able to load, double count hits
          if(verbose_flag) {
            //printf("| modify hit hit ");
            printf("hit ");
          }
          hit_count++;
          //hit_count++;
        } else {
          // If unable to load
          if(verbose_flag) {
            //printf("| modify miss ");
            printf("miss ");
          }
          miss_count++;
        }
        if(store(current_line_command, cache, s, E, b) > 0) {
          // If able to store, store
        } else {
          // If unable to store, evict count eviction
          if(verbose_flag) {
            //printf("| modify eviction ");
            printf("eviction ");
          }
          eviction_notice(current_line_command, cache, S, E, b);
          store(current_line_command, cache, s, E, b);
          eviction_count++;
        }
        // Always count hit
        if(verbose_flag) {
          //printf("| hit ");
          printf("hit ");
        }
        hit_count++;
      // If current command is instruction, ignore
      } else {

      }

      // Adds to age of all valid cache
      int i, j;
      for(i = 0; i < S; i++) {
        for(j = 0; j < E; j++) {
          if(cache[i][j].v) {
            cache[i][j].age++;
          }
        }
      }
      printf("End Command ");
      if(verbose_flag) {
        printf("\n");
      }
   }

   // Deallocate the cache
   for (i = 0; i < S; i++) {
     free(cache[i]);
   }
   free(cache);

  // Close file and print summary
  fclose(file_contents);
  printSummary(hit_count, miss_count, eviction_count);

  return 0;
}
