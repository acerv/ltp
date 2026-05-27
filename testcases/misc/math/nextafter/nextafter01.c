// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2002
 * Copyright (c) Linux Test Project, 2002-2026
 *   Ported to LTP: 01/02/2003 avenkat@us.ibm.com
 *   Ported to Linux: 06/30/2001 nsharoff@us.ibm.com
 */

/*\
 * Verify basic functionality of :manpage:`nextafter(3)`.
 *
 * - nextafter(1.0, 1.1) returns the next representable value after 1.0
 *   toward 1.1, and the midpoint between that value and 1.0 rounds
 *   correctly.
 * - nextafter(1.0, 0.9) returns the next representable value after 1.0
 *   toward 0.9, and the previously computed midpoint is consistent.
 * - nextafter(1.0, 1.0) returns exactly 1.0.
 */

#include <math.h>
#include "tst_test.h"

static void run(void)
{
	double answer, check;

	answer = nextafter(1.0, 1.1);
	check = (answer + 1.0) / 2;

	if (check != answer && (float)check != 1.0)
		tst_res(TFAIL, "nextafter(1.0, 1.1) midpoint check failed");
	else
		tst_res(TPASS, "nextafter(1.0, 1.1) returned %e", answer);

	answer = nextafter(1.0, 0.9);

	if (check != answer && check != 1.0)
		tst_res(TFAIL, "nextafter(1.0, 0.9) midpoint check failed");
	else
		tst_res(TPASS, "nextafter(1.0, 0.9) returned %e", answer);

	answer = nextafter(1.0, 1.0);

	if (answer != 1.0) {
		tst_res(TFAIL, "nextafter(1.0, 1.0) returned %e, expected 1.0",
			answer);
	} else {
		tst_res(TPASS, "nextafter(1.0, 1.0) returned 1.0");
	}
}

static struct tst_test test = {
	.test_all = run,
};
