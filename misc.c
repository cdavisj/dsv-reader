#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "misc.h"

/*
 * option processing using standard getopt (not the gnu version)
 * There is one required option -c col_cnt
 * We check all the args to find as many errors in the command line 
 * note getopt() passes values using global varibles: optarg, optind
 * these globals are declared in unistd.h
 */
int
do_opts(int argc, char **argv, int *in_colcnt, unsigned long *st_ln,
        unsigned long *end_ln, char *indelim, char *outdelim)
{
    int opt;            /* option flag returned by getopt */
    int err_opt = 0;    /* set to non-zero if one of the args is bad */
    int saw_c = 0;      /* did we see the manditory -c option */
    char *endptr;       /* for strtol(); ensure entire argument is parsed */
                        /* see man 3 strtol,strtoul for error handling */
    
    /*
     * set the defaults and/or initial values
     */
    *in_colcnt = -1;
    *st_ln = 1UL; 
    *end_ln = 0UL; 
    *indelim = CDELIM;
    *outdelim = CDELIM;

    /*
     * call getopt for each arg
     */
    while ((opt = getopt(argc, argv, "+c:s:e:i:o:")) != -1) {
        switch (opt) {
        case 'c':
            /*
             * number of columns in csv file must be greater than 0
             */
            errno = 0;
            if (((*in_colcnt = (int)strtol(optarg, &endptr, 10)) <= 0) 
                    || (*endptr != '\0') || (errno != 0)) {
                fprintf(stderr, "%s: -c operand must be larger than 0\n", *argv);
                err_opt = 1;
            }
            saw_c = 1;
            break;
        case 'e':
            /*
             * line to end output, default is 0 (all lines), 1 would be line 1 etc.
             */
            errno = 0;
            *end_ln = strtoul(optarg, &endptr, 10);
            if ((*endptr != '\0') || (errno != 0)) {
                fprintf(stderr, "%s: -e operand bad value\n", *argv);
                err_opt = 1;
            }
            break;
        case 'i':
            /*
             * change the input delimiter from the default to specified single char
             */
            if (*(optarg + 1) != '\0') {
                fprintf(stderr, "%s: -i operand must be a single char\n", *argv);
                err_opt = 1;
            }
            *indelim = *optarg;
            break;
        case 'o':
            /*
             * change the output delimiter from the default to specified single char
             */
            if (*(optarg + 1) != '\0') {
                fprintf(stderr, "%s: -o operand must be a single char\n", *argv);
                err_opt = 1;
            }
            *outdelim = *optarg;
            break;
        case 's':
            /*
             * line to start output, default is 1 (first line)
             */
            errno = 0;
            *st_ln = strtoul(optarg, &endptr, 10);
            if ((*endptr != '\0') || (errno != 0) || (st_ln == 0UL)) {
                fprintf(stderr, "%s: -s operand bad value\n", *argv);
                err_opt = 1;
            }
            break;
        case '?':
        default:
            /*
             * this is the case for error message handle directly by getopt()
             * we just mark we got an error
             */
            err_opt = 1;
            break;
        }
    }

    /*
     * make sure the user specified -c
     */
    if (!saw_c) {
        fprintf(stderr, "%s: -c operand is required\n", *argv);
        err_opt = 1;
    }

    /*
     * make sure the starting and ending lines range makes sense
     */
    if ((*end_ln > 0) && (*end_ln < *st_ln)) {
        fprintf(stderr, "%s: -s operand is larger than -e operand\n", *argv);
        err_opt = 1;
    }
    
    /*
     * make sure that there is at least one column specified in the output
     */
    if (optind == argc) {
        fprintf(stderr, "%s: Must specify at least one column\n", *argv);
        err_opt = 1;
    }

    /*
     * if bad or missing required option, print usage and exit
     */
    if (err_opt) {
        fprintf(stderr,
            "Usage: %s -c count [-s start] [-e end] [-i indelm] [-o outdelm] col [..]\n",
            *argv);
        return 0;
    }
    return optind;
}

/*
 * function for printing dropped error messages to stderr
 */
void
dropmsg(char *mesg, unsigned long lineno, char **argv)
{
    fprintf(stderr, "%s: drop line %lu, %s\n", *argv, lineno, mesg);
}
