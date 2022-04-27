#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "subs.h"
#include "misc.h"

/*
 * split_input
 *
 * usage:   input buffer csv format, builds table of pointers to each column
 * assumes: input buffer has at least one char in it (not counting the '\0')
 * args:
 *  buf     one record from read from standind input stream by getline()
 *  delim   the input field delimiter
 *  cnt     number of fields (in table)
 *  table   point to a table of pointers to the start of each record
 *  lineno  line number of record (for passing to dropmesg)
 *  argv    for drop mesg to print name of program
 * returns: 0 if all the columns were found,
 *          -1 if a record did not have all the columns
 *          -1 if a record had an bad field value
 */
int
split_input(char *buf, char delim, int cnt, char **table, unsigned long lineno,
            char **argv)
{
    int idx = 0;    /* index for table */
    int fldof = 0;  /* indicates if we have an overflow of fields */

    /*
     * walk through buffer, validating fields and directing table pointers to
     * them
     */
    while (*buf != '\0') {
        /* checks that we are not trying to add more fields than allowed */
        if (idx >= cnt) {
            fldof = 1;
            break;
        }

        /*
         * point next text table slot to start of next field before validating
         * that this field is valid. If it is not valid, record is dropped from
         * output
         */
        *(table + idx++) = buf;

        if (*buf == '"') {
            /*
             * if we have a quoted field, loop over field looking for invalid
             * character sequences. Invalid character sequences includes \0
             * before a terminating "; a " not followed by the delimiter, \n, or
             * another ".
             */
            while (*buf != '\0') {
                /* update buffer */
                buf++;

                /*
                 * if we have \0 before closing ", the quoted field was not
                 * properly terminated
                 */
                if (*buf == '\0') {
                    dropmsg("Quoted field missing final quote", lineno,
                                argv);
                    return -1;
                }

                /*
                 * if we hit a ", we must have it followed by the delimiter, a
                 * \n, or another ". Otherwise, it is an invalid field
                 */
                if (*buf == '"') {
                    if ((*(buf + 1) == delim) || (*(buf + 1) == '\n')) {
                        buf++;
                        *buf = '\0';
                        break;

                    } else if (*(buf + 1) == '"') {
                        buf++;

                    } else {
                        dropmsg("Quoted field not terminated properly", lineno,
                                    argv);
                        return -1;
                    }
                }
            }

        } else {
            /*
             * if not a quoted field first make sure it isn't an elmpty field.
             * If it is an empty field, then add null terminator and continue to
             * next field
             */
            if (*buf == delim || *buf == '\n') {
                *buf = '\0';

            } else {
                /*
                 * if we have an unquoted field, loop over field looking for invalid
                 * character sequences. The only invalid character sequence in this
                 * case is having " anywhere in the field
                 */
                while (*(buf++) != '\0') {
                    if (*buf == '"') {
                        dropmsg("A \" is not allowed inside unquoted field",
                                    lineno, argv);
                        return -1;

                    } else if (*buf == '\n' || *buf == delim) {
                        *buf = '\0';
                        break;
                    }
                }
            }
        }

        /* moves to next field */
        buf++;
    }

    /*
     * make sure we have the correct number of fields read. If we have too many
     * or too few, prints corresponding error message
     */
    if (idx < cnt) {
        dropmsg("too few columns", lineno, argv);
        return -1;

    } else if (fldof) {
        dropmsg("too many columns", lineno, argv);
        return -1;
    }

    return 0;
}

/*
 * wr_row
 *
 * usage:   given an array of pointers to columns and an array of columns to
 *          ouput and a count of output columns
 *          output those columns specified in the output array (contents are
 *          index numbers of the input array)
 * args:
 *  in_tab  pointer to a table of pointers to the start of each input field
 *  out_tab pointer to a table of ints containing the array index to print
 *          the order in this array is the order the cols/fields are printed
 *  out_cnt number of elements in out_tab
 *  delim   the output record delimiter to use
 *  lineno  line number of record for dropmsg
 *  argv    program name for dropmesg
 * return:  the number of records dropped during output
 *          0 if the record was printed
 *          1 if the record was dropped (special case 1 col and is empty)
 */
int
wr_row(char **in_tab, int *out_tab, int out_cnt, char delim,
        unsigned long lineno, char **argv)
{
    /*
     * check for empty columns on single column outputs. If one is found
     * prints nothing for that row
     */
    if ((out_cnt == 1) && (**(in_tab + *out_tab) == '\0')) {
        dropmsg("empty column", lineno, argv);
        return 1;
    }

    /*
     * iterate over out_tab array, grabbing indices for selected elements of
     * table, and printing corresponding fields
     *
     * includes newline to account for following records
     */
    for (int i = 0; i < out_cnt; i++) {
        if (i == out_cnt - 1) {
            printf("%s", *(in_tab + *(out_tab + i)));

        } else {
            printf("%s%c", *(in_tab + *(out_tab + i)), delim);
        }
    }
    printf("\n");

    return 0;
}
