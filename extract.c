/*
 * Usage ./extract -c col_cnt col#, ...
 *  col #'s start at 1
 *  dsv file on stdin, accepts only lines with exactly col_cnt columns 
 *  and lines with proper "" fields if any
 *  outputs the selected columns in the order specified.
 *  at least 1 output column must be selected
 *  output lines with one column and empty are dropped (no '\n' only lines)
 *
 *  assumes that file line counts can be contain in an unsigned long
 *  handling wrap of an unsigned long is not required for this PA
 *
 * error handling:
 *     print out error for bad arguments and malformed lines
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "subs.h"
#include "misc.h"

/*
 * main
 *
 * arguments:
 *  argc    number of arguments
 *  argv    array of argument values
 *
 * operation: driver function for csv column extract process reads a line from
 *  stdin, breaks the line into tokens writes to stdout the selected colums
 *  specified in arg order
 */
int
main(int argc, char **argv)
{
    int in_colcnt;              /* specified column count */
    int out_colcnt;             /* output column count */
    char **in_table;            /* array of pointers to column starts */
    int *out_table;             /* array of output column numbers to output */
    unsigned long linecnt;      /* input line/record number counter */
    char *buf = NULL;           /* input buffer, getline() will allocate it */
    size_t bufsz = 0;           /* size of buffer altered by getline() */
    char *endptr;               /* strtol() arg */
    char **colpt;               /* ptr to first col arg on the command line */
    int colargs;                /* index of first output column option */
    int *otpt;                  /* point for walking out_table */
    int saw_error;              /* was there an error with the column args */
    char indelim;               /* column/field delimiter input */
    char outdelim;              /* column/field delimiter output */
    unsigned long st_ln;        /* first line/record number to process */
    unsigned long end_ln;       /* last line/record number, 0 is to EOF */
    unsigned long dpcnt = 0;    /* number of rows dropped in total */
    int retval = EXIT_SUCCESS;  /* return value */

    /*
     * parse the command line args.
     * if there was an error, return with EXIT_FAILURE
     */
    if ((colargs = do_opts(argc, argv, &in_colcnt, &st_ln, &end_ln,
        &indelim, &outdelim)) == 0)
        return EXIT_FAILURE;

    /*
     * allocate space for in_table which is an array of pointers to the start
     * of each column in an input record after it is broken into tokens
     * if unable to allocate, return with EXIT_FAILURE
     */
    if ((in_table = malloc(in_colcnt * sizeof(*in_table))) == NULL) {
        fprintf(stderr, "%s: malloc() failed input table\n", *argv);
        return EXIT_FAILURE;
    }

    /*
     * allocate space for out_table which is an array of indexes specifying rows
     * in in_table. Each entry contains an row index integer of a row in the
     * in_table. The index numbers specify the columns and order in which they
     * will be written in an output record. This allows specifying subsets of
     * the input record columns changing their order in an output record.
     * You can drop and repeat any of the input record columns.
     * if unable to allocate, return with EXIT_FAILURE
     */
    out_colcnt = argc - colargs;
    if ((out_table = malloc(out_colcnt * sizeof(out_table))) == NULL) {
        fprintf(stderr, "%s: malloc() failed output table\n", *argv);
        free(in_table);
        return EXIT_FAILURE;
    }

    /*
     * build the output column table with the list of csv columns to print
     * check all column args, do not stop at first error
     */
    saw_error = 0;
    otpt = out_table;
    colpt = argv + colargs;

    while (*colpt != NULL) {
        /*
         * translate column number to index slot offset
         * range check each output column number to make sure it is
         * between 0 and the in_colcnt-1 (adjusted for index numbers)
         */
        errno = 0;
        *otpt = (int) strtol(*colpt, &endptr, 10);
        if ((*endptr != '\0') || (errno != 0) ||
            (*otpt < 1 || *otpt > in_colcnt)) {
            fprintf(stderr, "%s: invalid column number\n", *argv);
            saw_error = 1;
        }
        (*otpt)--;

        otpt++;
        colpt++;
    }

    /*
     * check all the args if there was any bad column args
     * we are done, return EXIT_FAILURE
     */
    if (saw_error != 0) {
        /* free malloc-allocated memory */
        free(in_table);
        free(out_table);
        return EXIT_FAILURE;
    }

    /*
     * read the input line at a time, break into tokens and write out the
     * selected columns
     */
    linecnt = 0;

    while ((getline(&buf, &bufsz, stdin)) > 0) {
        /*
         * update linecnt and skip until we reach first specified input line
         */
        linecnt++;

        if (linecnt < st_ln)
            continue;

        /*
         * break the input into tokens point at by array in_table
         * then write it out
         */
        if (split_input(buf, indelim, in_colcnt, in_table, linecnt, argv) != 0)
            dpcnt++;    /* count the dropped records */
        else
            dpcnt += wr_row(in_table, out_table, out_colcnt, outdelim,
                             linecnt, argv);

        /*
         * if we've reached the last line specified, we're done
         */
        if ((end_ln > 0) && (linecnt == end_ln))
            break;
    }

    /*
     * if there were less lines in the input than specified, alert the user
     */
    if ((end_ln > 0) && (linecnt < end_ln - st_ln)) {
        fprintf(stderr,
            "%s: Input less lines (%lu) than specified -e (%lu)\n",
            *argv, linecnt, end_ln);
        retval = EXIT_FAILURE;
    }

    /*
     * if lines were dropped, write summary to stderr
     */
    if (dpcnt > 0) {
        /* show only lines processed, accounts for range skipping */
        if (end_ln > 0)
            linecnt = end_ln - st_ln + 1;
        else
            linecnt -= (st_ln - 1);

        fprintf(stderr, "%s: %lu dropped out of %lu processed\n", *argv,
                dpcnt, linecnt);
        retval = EXIT_FAILURE;
    }


    /*
     * free memory for in_table and out_table (since allocated with malloc)
     */
    free(in_table);
    free(out_table);

    return retval;
}
