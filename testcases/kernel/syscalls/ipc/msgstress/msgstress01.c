// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2002
 *   06/30/2001   Port to Linux   nsharoff@us.ibm.com
 *   11/11/2002   Port to LTP     dbarrera@us.ibm.com
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*
 * [Description]
 *
 * Stress test for SysV IPC. We send multiple messages at the same time,
 * checking that we are not loosing any byte once we receive the messages.
 */

#include <stdlib.h>
#include "tst_safe_sysv_ipc.h"
#include "tst_safe_stdio.h"
#include "tst_test.h"

#define SYSVIPC_FILE "/proc/sysvipc/msg"
#define KEY_VAL0 154326L
#define MSGTYPE 10

struct sysv_msg {
	long type;
	struct {
		char len;
		char pbytes[99];
	} data;
};

struct sysv_data {
	int id;
	struct sysv_msg msg;
};

static char *str_num_messages;
static int num_messages = 1000;

static struct sysv_data *ipc_data;
static int ipc_data_len;

static int get_used_sysvipc(void)
{
	FILE *fp;
	int used = -1;
	char buf[BUFSIZ];

	fp = SAFE_FOPEN(SYSVIPC_FILE, "r");

	while (fgets(buf, BUFSIZ, fp) != NULL)
		used++;

	SAFE_FCLOSE(fp);

	if (used < 0)
		tst_brk(TBROK, "Can't read %s to get used sysvipc resource", SYSVIPC_FILE);

	return used;
}

static int create_message(const int id)
{
	int pos = ipc_data_len;
	struct sysv_data *buff = ipc_data + pos;

	buff->id = id;
	buff->msg.type = MSGTYPE;
	buff->msg.data.len = (rand() % 99) + 1;

	for (int i = 0; i < buff->msg.data.len; i++)
		buff->msg.data.pbytes[i] = rand() % 255;

	ipc_data_len++;

	return pos;
}

static void reader(const int id)
{
	int size;
	struct sysv_msg msg_recv;
	struct sysv_data *buff = NULL;

	memset(&msg_recv, 0, sizeof(struct sysv_msg));

	size = SAFE_MSGRCV(id, &msg_recv, 100, MSGTYPE, 0);

	for (int i = 0; i < ipc_data_len; i++) {
		if (ipc_data[i].id == id) {
			buff = ipc_data + i;
			break;
		}
	}

	if (!buff) {
		tst_brk(TBROK, "Can't find original message. This is a test issue!");
		return;
	}

	TST_EXP_EQ_LI(msg_recv.type, buff->msg.type);
	TST_EXP_EQ_LI(msg_recv.data.len, buff->msg.data.len);

	for (int i = 0; i < size; i++) {
		if (msg_recv.data.pbytes[i] != buff->msg.data.pbytes[i]) {
			tst_res(TFAIL, "Received wrong data at index %d: %x != %x", i,
				msg_recv.data.pbytes[i],
				buff->msg.data.pbytes[i]);

			return;
		}
	}

	tst_res(TPASS, "Received correct data");
}

static void run(void)
{
	int id, pos;

	ipc_data_len = 0;
	memset(ipc_data, 0, sizeof(struct sysv_data) * num_messages);

	for (int i = 0; i < num_messages; i++) {
		id = SAFE_MSGGET(KEY_VAL0 + i, IPC_CREAT | 0600);
		pos = create_message(id);

		if (!SAFE_FORK()) {
			struct sysv_data *buff = &ipc_data[pos];

			SAFE_MSGSND(id, &buff->msg, 100, 0);
			return;
		}

		if (!SAFE_FORK()) {
			reader(id);
			return;
		}
	}
}

static void setup(void)
{
	int total_msg, avail_msg;

	srand(time(0));

	SAFE_FILE_SCANF("/proc/sys/kernel/msgmni", "%i", &total_msg);

	avail_msg = total_msg - get_used_sysvipc();

	tst_res(TINFO, "Available messages slots: %d", avail_msg);

	if (tst_parse_int(str_num_messages, &num_messages, 1, avail_msg))
		tst_brk(TBROK, "Invalid number of messages '%s'", str_num_messages);

	ipc_data = SAFE_MMAP(
		NULL,
		sizeof(struct sysv_data) * num_messages,
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);
}

static void cleanup(void)
{
	SAFE_MUNMAP(ipc_data, sizeof(struct sysv_data) * num_messages);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{"n:", &str_num_messages, "Number of messages to send (default: 1000)"},
		{},
	},
};
