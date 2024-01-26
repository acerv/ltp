// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *     07/2001 Ported by Wayne Boyer
 * Copyright (C) 2024 Andrea Cervesato andrea.cervesato@suse.com
 */

/*\
 * [Description]
 *
 * This test is checking fcntl() syscall locking mechanism between two
 * processes.
 * The test sets a random starting position on file using lseek(), it randomly
 * generates fcntl() parameters for parent and child and it verifies that
 * fcntl() will raise a blocking error on child when it's supposed to.
 */

#include <fcntl.h>
#include <stdlib.h>
#include "tst_test.h"

#ifndef S_ENFMT
# define S_ENFMT S_ISGID
#endif

#define CHECK_EQ_SILENT_(VAL_A, SVAL_A, VAL_B, SVAL_B, TYPE, PFS) ({\
	int failure = 0; \
	TYPE tst_tmp_a__ = VAL_A; \
	TYPE tst_tmp_b__ = VAL_B; \
	if (tst_tmp_a__ != tst_tmp_b__) { \
		tst_res(TFAIL, \
			SVAL_A " == " SVAL_B " (" PFS ")", tst_tmp_a__); \
			failure = 1; \
	} \
	failure; \
})

#define CHECK_EQ_SILENT(VAL_A, VAL_B) \
		CHECK_EQ_SILENT_(VAL_A, #VAL_A, VAL_B, #VAL_B, long long, "%lli")

struct file_conf {
	short type;
	short whence;
	long start;
	long len;
};

struct testcase {
	struct file_conf parent;           /* parent parameters for fcntl() */
	struct file_conf child;            /* child parameters for fcntl() */
	short blocking;                    /* blocking/non-blocking flag */
	long pos;                          /* starting file position */
};

struct tst_results {
	int num_pass;
	int last_failed;
};

static const char filepath[] = "unlocked.txt";
static const char filedata[] = "Here some bytes!";
static struct flock flock;
static char *str_op_nums;
static char *locking_file;
static int op_nums = 5000;
static int file_mode = 0777;
static struct tst_results *results;

static void dochild(struct testcase *tc, const int fd, const pid_t parent_pid)
{
	results->last_failed = 0;

	flock.l_type = tc->child.type;
	flock.l_whence = tc->child.whence;
	flock.l_start = tc->child.start;
	flock.l_len = tc->child.len;
	flock.l_pid = 0;

	SAFE_FCNTL(fd, F_GETLK, &flock);

	if (tc->blocking) {
		tst_res(TDEBUG, "Child: expecting blocked file by parent");

		results->last_failed = CHECK_EQ_SILENT(flock.l_pid, parent_pid);
		if (results->last_failed)
			return;

		results->last_failed = CHECK_EQ_SILENT(flock.l_type, tc->parent.type);
		if (results->last_failed)
			return;

		flock.l_type = tc->child.type;
		flock.l_whence = tc->child.whence;
		flock.l_start = tc->child.start;
		flock.l_len = tc->child.len;
		flock.l_pid = 0;

		TST_EXP_FAIL_SILENT(fcntl(fd, F_SETLK, &flock), EWOULDBLOCK);
		if (TST_RET != -1)
			results->last_failed = 1;
	} else {
		tst_res(TDEBUG, "Child: expecting no blocking errors");

		results->last_failed = CHECK_EQ_SILENT(flock.l_type, F_UNLCK);
		if (results->last_failed)
			return;

		results->last_failed = CHECK_EQ_SILENT(flock.l_whence, tc->child.whence);
		if (results->last_failed)
			return;

		results->last_failed = CHECK_EQ_SILENT(flock.l_start, tc->child.start);
		if (results->last_failed)
			return;

		results->last_failed = CHECK_EQ_SILENT(flock.l_len, tc->child.len);
		if (results->last_failed)
			return;

		results->last_failed = CHECK_EQ_SILENT(flock.l_pid, 0);
		if (results->last_failed)
			return;

		TST_EXP_PASS_SILENT(fcntl(fd, F_SETLK, &flock));
		if (TST_RET == -1)
			results->last_failed = 1;
	}
}

static void run_testcase(struct testcase *tc, const int file_mode)
{
	pid_t parent_pid;
	pid_t child_pid;
	int fd;

	tst_res(TDEBUG, "Parent: locking file");

	/* open file and move cursor according with the test */
	fd = SAFE_OPEN(filepath, O_RDWR, file_mode);
	SAFE_LSEEK(fd, tc->pos, 0);

	/* set the initial parent lock on the file */
	flock.l_type = tc->parent.type;
	flock.l_whence = tc->parent.whence;
	flock.l_start = tc->parent.start;
	flock.l_len = tc->parent.len;
	flock.l_pid = 0;

	SAFE_FCNTL(fd, F_SETLK, &flock);

	/* set the child lock on the file */
	parent_pid = getpid();
	child_pid = SAFE_FORK();

	if (!child_pid) {
		dochild(tc, fd, parent_pid);
		exit(0);
	}

	tst_reap_children();

	flock.l_type = F_UNLCK;
	flock.l_whence = 0;
	flock.l_start = 0;
	flock.l_len = 0;
	flock.l_pid = 0;

	SAFE_FCNTL(fd, F_SETLK, &flock);
	SAFE_CLOSE(fd);
}

static void genconf(struct file_conf *conf, const int size, const long pos)
{
	conf->type = rand() % 2 ? F_RDLCK : F_WRLCK;
	conf->whence = SEEK_CUR;

	if (pos > 0 && (rand() % 2)) {
		conf->start = -(rand() % pos);
		conf->len = rand() % (size + conf->start - 1) + 1;
	} else {
		conf->start = rand() % (size - 1);
		conf->len = rand() % (size - conf->start - 1) + 1;
	}
}

static short fcntl_overlap(
	struct file_conf *parent,
	struct file_conf *child,
	const long pos)
{
	long start[2];
	long length[2];
	short overlap = 0;

	if (parent->start > child->start) {
		start[0] = pos + child->start;
		start[1] = pos + parent->start;
		length[0] = child->len;
		length[1] = parent->len;
	} else {
		start[0] = pos + parent->start;
		start[1] = pos + child->start;
		length[0] = parent->len;
		length[1] = child->len;
	}

	overlap = start[0] <= start[1] && start[1] < (start[0] + length[0]);

	if (overlap)
		tst_res(TDEBUG, "child/parent fcntl() configurations overlap");

	return overlap;
}

static void gentestcase(struct testcase *tc)
{
	struct file_conf *parent = &tc->parent;
	struct file_conf *child = &tc->child;
	int size;

	size = strlen(filedata);
	tc->pos = rand() % size;

	genconf(parent, size, tc->pos);
	genconf(child, size, tc->pos);

	tc->blocking = fcntl_overlap(parent, child, tc->pos);

	if (parent->type == F_RDLCK && child->type == F_RDLCK)
		tc->blocking = 0;
}

static void print_testcase(struct testcase *tc)
{
	tst_res(TDEBUG, "Starting read/write position: %ld", tc->pos);

	tst_res(TDEBUG,
		"Parent: type=%s whence=%s start=%ld len=%ld",
		tc->parent.type == F_RDLCK ? "F_RDLCK" : "F_WRLCK",
		tc->parent.whence == SEEK_SET ? "SEEK_SET" : "SEEK_CUR",
		tc->parent.start,
		tc->parent.len);

	tst_res(TDEBUG,
		"Child: type=%s whence=%s start=%ld len=%ld",
		tc->child.type == F_RDLCK ? "F_RDLCK" : "F_WRLCK",
		tc->child.whence == SEEK_SET ? "SEEK_SET" : "SEEK_CUR",
		tc->child.start,
		tc->child.len);

	tst_res(TDEBUG, "Expencted %s test",
		tc->blocking ? "blocking" : "non-blocking");
}

static void run(void)
{
	struct testcase tc;

	results->num_pass = 0;

	for (int i = 0; i < op_nums; i++) {
		gentestcase(&tc);
		print_testcase(&tc);

		tst_res(TDEBUG, "Running test #%u", i);
		run_testcase(&tc, file_mode);

		if (results->last_failed)
			return;

		results->num_pass++;
	}

	if (results->num_pass == op_nums)
		tst_res(TPASS, "All %d test file operations passed", op_nums);
}

static void setup(void)
{
	int fd;

	if (tst_parse_int(str_op_nums, &op_nums, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of operations '%s'", str_op_nums);

	if (locking_file) {
		tst_res(TINFO, "Requested mandatory locking");

		file_mode = S_ENFMT | 0600;
	}

	fd = SAFE_OPEN(filepath, O_CREAT | O_RDWR | O_TRUNC, 0777);
	SAFE_WRITE(SAFE_WRITE_ALL, fd, filedata, strlen(filedata));
	SAFE_CLOSE(fd);

	srand(time(0));

	results = SAFE_MMAP(
		NULL,
		sizeof(struct tst_results),
	    PROT_READ | PROT_WRITE,
		MAP_ANONYMOUS | MAP_SHARED,
		-1, 0);
}

static void cleanup(void)
{
	if (results)
		SAFE_MUNMAP(results, sizeof(struct tst_results));
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.forks_child = 1,
	.needs_tmpdir = 1,
	.options = (struct tst_option[]) {
		{ "n:", &str_op_nums, "Total # operations to do (default 5000)" },
		{ "l", &locking_file, "Set locking flag on file" },
		{},
	},
	.skip_filesystems = (const char *const []) {
		"nfs",
		NULL
	},
};
