/*
 * file_c.c -- File cache management 
 */

#include <dirent.h>
#include "copyright.h"
#include "config.h"

#include "config.h"
#include "interface.h"
#include "mudconf.h"
#include "command.h"
#include "file_c.h"
#include "alloc.h"

typedef struct filecache_hdr FCACHE;
typedef struct filecache_block_hdr FBLKHDR;
typedef struct filecache_block FBLOCK;

struct filecache_hdr {
    char *filename;
    FBLOCK *fileblock;
    const char *desc;
};

struct filecache_block {
    struct filecache_block_hdr {
	struct filecache_block *nxt;
	int nchars;
    } hdr;
    char data[MBUF_SIZE - sizeof(FBLKHDR)];
};

#define FBLOCK_SIZE (MBUF_SIZE - sizeof(FBLKHDR))

FCACHE fcache[] = {
	{ mudconf.conn_file,	NULL,	"Conn" },
	{ mudconf.site_file,	NULL,	"Conn/Badsite" },
	{ mudconf.down_file,	NULL,	"Conn/Down" },
	{ mudconf.full_file,	NULL,	"Conn/Full" },
	{ mudconf.guest_file,	NULL,	"Conn/Guest" },
	{ mudconf.creg_file,	NULL,	"Conn/Reg" },
	{ mudconf.crea_file,	NULL,	"Crea/Newuser" },
	{ mudconf.regf_file,	NULL,	"Crea/RegFaill" },
	{ mudconf.motd_file,	NULL,	"Motd" },
	{ mudconf.wizmotd_file,	NULL,	"Wizmotd" },
	{ mudconf.quit_file,	NULL,	"Quit" },
	{ NULL,			NULL,	NULL }};

NAMETAB list_files[] = {
{(char *)"badsite_connect",	1,	CA_WIZARD,	FC_CONN_SITE},
{(char *)"connect",		2,	CA_WIZARD,	FC_CONN},
{(char *)"create_register",	2,	CA_WIZARD,	FC_CREA_REG},
{(char *)"down",		1,	CA_WIZARD,	FC_CONN_DOWN},
{(char *)"full",		1,	CA_WIZARD,	FC_CONN_FULL},
{(char *)"guest_motd",		1,	CA_WIZARD,	FC_CONN_GUEST},
{(char *)"motd",		1,	CA_WIZARD,	FC_MOTD},
{(char *)"newuser",		1,	CA_WIZARD,	FC_CREA_NEW},
{(char *)"quit",		1,	CA_WIZARD,	FC_QUIT},
{(char *)"register_connect",	1,	CA_WIZARD,	FC_CONN_REG},
{(char *)"wizard_motd",		1,	CA_WIZARD,	FC_WIZMOTD},
{ NULL,				0,	0,		0}};

#define MAX_CONN 100
int fcache_conn_c = 0;
FCACHE fcache_conn[MAX_CONN];

void do_list_file(dbref player, dbref cause, int extra, char *arg)
{
    int flagvalue;

    flagvalue = search_nametab(player, list_files, arg);
    if (flagvalue < 0) {
	display_nametab(player, list_files,
	    (char *) "Unknown file.  Use one of:", 1);
	return;
    }
    fcache_send(player, flagvalue);
}

static FBLOCK *fcache_fill(FBLOCK *fp, char ch)
{
    FBLOCK *tfp;

    if (fp->hdr.nchars >= (MBUF_SIZE - sizeof(FBLKHDR))) {

	/*
	 * We filled the current buffer.  Go get a new one. 
	 */

	tfp = fp;
	fp = (FBLOCK *) alloc_mbuf("fcache_fill");
	fp->hdr.nxt = NULL;
	fp->hdr.nchars = 0;
	tfp->hdr.nxt = fp;
    }
    fp->data[fp->hdr.nchars++] = ch;
    return fp;
}

static int fcache_read(FBLOCK **cp, char *filename)
{
    int n, nmax, tchars, fd;
    char *buff;
    FBLOCK *fp, *tfp;

    /*
     * Free a prior buffer chain 
     */

    fp = *cp;
    while (fp != NULL) {
	tfp = fp->hdr.nxt;
	free_mbuf(fp);
	fp = tfp;
    }
    *cp = NULL;

    /*
     * Read the text file into a new chain 
     */

    if ((fd = open(filename, O_RDONLY)) == -1) {

	/*
	 * Failure: log the event 
	 */

	STARTLOG(LOG_PROBLEMS, "FIL", "OPEN") {
	    buff = alloc_mbuf("fcache_read.LOG");
	    sprintf(buff, "Couldn't open file '%s'.", filename);
	    log_text(buff);
	    free_mbuf(buff);
	    ENDLOG;
	}

	return -1;
    }
    buff = alloc_lbuf("fcache_read.temp");

    /*
     * Set up the initial cache buffer to make things easier 
     */

    fp = (FBLOCK *) alloc_mbuf("fcache_read.first");
    fp->hdr.nxt = NULL;
    fp->hdr.nchars = 0;
    *cp = fp;
    tchars = 0;

    /*
     * Process the file, one lbuf at a time 
     */

    nmax = read(fd, buff, LBUF_SIZE);
    while (nmax > 0) {

	for (n = 0; n < nmax; n++) {
	    switch (buff[n]) {
	    case '\n':
		fp = fcache_fill(fp, '\r');
		fp = fcache_fill(fp, '\n');
		tchars += 2;
	    case '\0':
	    case '\r':
		break;
	    default:
		fp = fcache_fill(fp, buff[n]);
		tchars++;
	    }
	}
	nmax = read(fd, buff, LBUF_SIZE);
    }
    free_lbuf(buff);
    close(fd);

    /*
     * If we didn't read anything in, toss the initial buffer 
     */

    if (fp->hdr.nchars == 0) {
	*cp = NULL;
	free_mbuf(fp);
    }
    return tchars;
}

void fcache_read_dir(char *dir, FCACHE foo[], int *cnt, int max)
{
    DIR *d;
    struct dirent *de;
    char buf[LBUF_SIZE];

    bzero(&foo[0], sizeof(FCACHE) * max);
    if (!(d = opendir(dir)))
	return;
    for (*cnt = 0; *cnt < max;) {
	if (!(de = readdir(d)))
	    break;
	if (de->d_name[0] == '.')
	    continue;
	if (!strstr(de->d_name, ".txt"))
	    continue;
	sprintf(buf, "%s/%s", dir, de->d_name);
	fcache_read(&(foo[*cnt].fileblock), buf);
	(*cnt)++;
    }
    closedir(d);
}

void fcache_rawdump(int fd, int num)
{
    int cnt, remaining;
    char *start;
    FBLOCK *fp;

    if ((num < 0) || (num > FC_LAST))
	return;
    fp = fcache[num].fileblock;

    while (fp != NULL) {
	start = fp->data;
	remaining = fp->hdr.nchars;
	while (remaining > 0) {

	    cnt = WRITE(fd, start, remaining);
	    if (cnt < 0)
		return;
	    remaining -= cnt;
	    start += cnt;
	}
	fp = fp->hdr.nxt;
    }
    return;
}

void fcache_dumpbase(DESC *d, FCACHE fc[], int num)
{
    FBLOCK *fp;

    fp = fc[num].fileblock;

    while (fp != NULL) {
	queue_write(d, fp->data, fp->hdr.nchars);
	fp = fp->hdr.nxt;
    }
}


void fcache_dump(DESC *d, int num)
{
    if ((num < 0) || (num > FC_LAST))
	return;
    fcache_dumpbase(d, fcache, num);
}

void fcache_dump_conn(DESC *d, int num)
{
    fcache_dumpbase(d, fcache_conn, num);
}


void fcache_send(dbref player, int num)
{
    DESC *d;

    DESC_ITER_PLAYER(player, d) {
	fcache_dump(d, num);
    }
}

void fcache_load(dbref player)
{
    FCACHE *fp;
    char *buff, *bufc, *sbuf;
    int i;

    buff = bufc = alloc_lbuf("fcache_load.lbuf");
    sbuf = alloc_sbuf("fcache_load.sbuf");
    for (fp = fcache; fp->filename; fp++) {
	i = fcache_read(&fp->fileblock, fp->filename);
	if ((player != NOTHING) && !Quiet(player)) {
	    sprintf(sbuf, "%d", i);
	    if (fp == fcache)
		safe_str((char *) "File sizes: ", buff, &bufc);
	    else
		safe_str((char *) "  ", buff, &bufc);
	    safe_str((char *) fp->desc, buff, &bufc);
	    safe_str((char *) "...", buff, &bufc);
	    safe_str(sbuf, buff, &bufc);
	}
    }
    *bufc = '\0';
    if (*mudconf.conn_dir)
	fcache_read_dir(mudconf.conn_dir, fcache_conn, &fcache_conn_c,
	    MAX_CONN);
    if ((player != NOTHING) && !Quiet(player)) {
	notify(player, buff);
    }
    free_lbuf(buff);
    free_sbuf(sbuf);
}

void fcache_init(void)
{
    FCACHE *fp;

    for (fp = fcache; fp->filename; fp++) {
	fp->fileblock = NULL;
    }
    fcache_load(NOTHING);
}
