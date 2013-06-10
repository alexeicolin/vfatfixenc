#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iconv.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <sys/queue.h>

/* Print the hexadecimal bytes. */

int verbose = 0;

void
showhex (const char * what, const char * a, int len)
{
    int i;
    printf ("%s: ", what);
    for (i = 0; i < len; i++) {
	printf ("%02X", (unsigned char) a[i]);
	if (i < len - 1)
	    printf (" ");
    }
    printf ("\n");
}

/* Display values, for the purpose of showing what this is doing. */

void
show_values (const char * before_after,
	     const char * in_start,   int len_start,
	     const char * out_start,  int outlen_start)
{
    if (!verbose)
        return;
    printf ("%s:\n", before_after);
    showhex ("IN string", in_start, len_start);
    showhex ("OUT string", out_start, outlen_start);
}

/* Initialize the library. */

iconv_t
initialize (const char *from_enc, const char *to_enc)
{
    iconv_t conv_desc;
    conv_desc = iconv_open (to_enc, from_enc);
    if ((int) conv_desc == -1) {
	/* Initialization failure. */
	if (errno == EINVAL) {
	    fprintf (stderr,
		     "Conversion from '%s' to '%s' is not supported.\n",
		     from_enc, to_enc);
	} else {
	    fprintf (stderr, "Initialization failure: %s\n",
		     strerror (errno));
	}
	exit (1);
    }
    return conv_desc;
}


/* Convert EUC into UTF-8 using the iconv library. */

char *
convert (iconv_t conv_desc, char * in, unsigned int * len_ptr)
{
    size_t iconv_value;
    char * out;
    unsigned int len = *len_ptr;
    unsigned int outlen;
    char * outstart;
    const char * in_start;
    int len_start;
    int outlen_start;

    /* Assign enough space to put the UTF-8. */
    outlen = 2*len;
    out = calloc (outlen, 1);
    /* Keep track of the variables. */
    len_start = len;
    outlen_start = outlen;
    outstart = out;
    in_start = in;
    /* Display what is in the variables before calling iconv. */
    show_values ("before",
		 in_start, len_start,
		 outstart, outlen_start);
    iconv_value = iconv (conv_desc, & in, & len, & out, & outlen);
    /* Handle failures. */
    if (iconv_value == (size_t) -1) {
	fprintf (stderr, "iconv failed: in string '%s', length %d, "
		"out string '%s', length %d\n",
		 in, len, outstart, outlen);
	switch (errno) {
	    /* See "man 3 iconv" for an explanation. */
	case EILSEQ:
	    fprintf (stderr, "Invalid multibyte sequence.\n");
	    break;
	case EINVAL:
	    fprintf (stderr, "Incomplete multibyte sequence.\n");
	    break;
	case E2BIG:
	    fprintf (stderr, "No more room.\n");
	    break;
	default:
	    fprintf (stderr, "Error: %s.\n", strerror (errno));
	}
	return NULL;
    }
    /* Display what is in the variables after calling iconv. */
    show_values ("after",
		 in_start, len_start,
		 outstart, outlen_start);
    *len_ptr = outlen_start - outlen;
    return outstart;
}

/* Close the connection with the library. */

void
finalize (iconv_t conv_desc)
{
    int v;
    v = iconv_close (conv_desc);
    if (v != 0) {
	fprintf (stderr, "iconv_close failed: %s\n", strerror (errno));
	exit (1);
    }
}

// Caller must free
char *
fix(char * in_string)
{
    unsigned int len;
    char * utf8_string, * utf16_string;
    iconv_t conv_desc;

    len = strlen (in_string);
    if (!len)
        return calloc(1,1);

    conv_desc = initialize ("UTF-8", "UTF-16LE");
    utf16_string = convert (conv_desc, in_string, &len);
    finalize (conv_desc);

    if (!utf16_string)
        return NULL;

    conv_desc = initialize ("UTF-16LE", "ISO-8859-1");
    utf8_string = convert (conv_desc, utf16_string, &len);
    finalize (conv_desc);

    if (!utf8_string)
        return NULL;

    free(utf16_string);
    return utf8_string;
}

int
is_dir(const char *path)
{
    struct stat dir_stat;
    if (lstat( path, &dir_stat) < 0)
    {
        perror (path);
        return 0;
    }
    return S_ISDIR(dir_stat.st_mode);
}

int main (int argc, char **argv )
{
    char *fixed_fname;
    char *fname;
    char fixed_path[PATH_MAX];
    char old_path[PATH_MAX];
    char *in_path;
    int dry_run = 0;
    char cmd[2]; // one-character command

	DIR* dir = NULL;
	struct dirent *entry;

    TAILQ_HEAD(tailhead, entry) head;
    struct tailhead *headp;                 /* Tail queue head. */
    struct entry {
            TAILQ_ENTRY(entry) entries;         /* Tail queue. */
            char path[PATH_MAX];
            char fname[PATH_MAX];
    } *n, *n2;

    if (argc != 2 && argc != 3) {
        printf("Usage: ./fixenc [-d] filename\n");
        return 1;
    }

    if (!strcmp(argv[1], "-d")) {
        in_path = argv[2];
        dry_run = 1;
    } else {
        in_path = argv[1];
    }

    TAILQ_INIT(&head);                      /* Initialize the queue. */

    n = calloc(sizeof(struct entry), 1);      /* Insert at the head. */
    strncpy(n->path, in_path, PATH_MAX);
    // NULL fname
    TAILQ_INSERT_HEAD(&head, n, entries);

    while (n != NULL) {
        TAILQ_REMOVE(&head, n, entries);

        fixed_fname = fix(n->fname);
        if (!fixed_fname) {
            printf("Conversion failed. Use old ('%s')? (y/n): ", n->fname);
            if (1 != scanf("%1s", cmd)) {
                printf("Failed to get user input\n");
                return 1;
            }
            printf("\n");
            if (!strcmp(cmd, "y"))
                fixed_fname = strdup(n->fname);
            else
               return 1;
        }

        bzero(fixed_path, PATH_MAX);
        strcat(fixed_path, n->path);
        strcat(fixed_path, "/");
        strcat(fixed_path, fixed_fname);

        printf("%s\n", fixed_path);

        if (!dry_run && fixed_fname && strcmp(n->fname, fixed_fname)) {
            bzero(old_path, PATH_MAX);
            strcat(old_path, n->path);
            strcat(old_path, "/");
            strcat(old_path, n->fname);

            if (rename(old_path, fixed_path)) {
                perror("rename");
                return 1;
            }
        }

        if (is_dir(fixed_path)) {
            dir = opendir(fixed_path);
            if (dir == NULL) {
                perror("opendir");
                printf("Error opening directory: '%s'", fixed_path);
                return 1;
            }
            while ((entry = readdir(dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") ||
                    !strcmp(entry->d_name, ".."))
                    continue;

                n2 = calloc(sizeof(struct entry), 1);
                strcpy(n2->path, fixed_path);
                strcpy(n2->fname, entry->d_name);
                TAILQ_INSERT_TAIL(&head, n2, entries);
            }
        }

        free(fixed_fname);
        free(n);
        n = head.tqh_first;
    }

    return 0;
}
