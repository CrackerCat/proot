/* -*- c-set-style: "K&R"; c-basic-offset: 8 -*-
 *
 * This file is part of WioM.
 *
 * Copyright (C) 2014 STMicroelectronics
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include <sched.h>	/* CLONE_THREAD, */
#include <unistd.h>	/* dup(2), */
#include <stdio.h>	/* fdopen(3), fprintf(3), */
#include <assert.h>	/* assert(3), */
#include <talloc.h>	/* talloc(3), */

#include "extension/wiom/wiom.h"
#include "extension/wiom/format.h"
#include "cli/note.h"

/**
 * Report all events that were stored in @config->history.
 */
void report_events_text(int fd, const Event *history)
{
	int new_fd = -1;
	FILE *file = NULL;

	size_t length;
	size_t i;
	int status;

	if (history == NULL)
		return;

	/* Duplicate fd because of fclose() at the end.  */
	new_fd = dup(fd);
	if (new_fd < 0) {
		note(NULL, ERROR, SYSTEM, "can't duplicate output file descriptor");
		return;
	}

	file = fdopen(new_fd, "w");
	if (file == NULL) {
		note(NULL, ERROR, SYSTEM, "can't use output file descriptor");
		goto end;
	}

	length = talloc_array_length(history);
	for (i = 0; i < length; i++) {
		const Event *event = &history[i];
		switch (event->action) {
#define CASE(a) case a:							\
			status = fprintf(file, "%d %s %s\n", event->pid, #a, event->load.path); \
			break;						\

		CASE(TRAVERSES)
		CASE(CREATES)
		CASE(DELETES)
		CASE(GETS_METADATA_OF)
		CASE(SETS_METADATA_OF)
		CASE(GETS_CONTENT_OF)
		CASE(SETS_CONTENT_OF)
		CASE(EXECUTES)
#undef CASE

		case MOVE_CREATES:
			status = fprintf(file, "%d MOVE_CREATES %s to %s\n", event->pid,
					event->load.path, event->load.path2);
			break;

		case MOVE_OVERRIDES:
			status = fprintf(file, "%d MOVE_OVERRIDES %s to %s\n", event->pid,
					event->load.path, event->load.path2);
			break;

		case CLONED:
			status = fprintf(file, "%d CLONED (%s) into %d\n", event->pid,
					(event->load.flags & CLONE_THREAD) != 0
					? "thread" : "process",
					event->load.new_pid);
			break;

		case EXITED:
			status = fprintf(file, "%d EXITED (status = %ld)\n", event->pid,
					event->load.status);
			break;

		default:
			assert(0);
			break;
		}

		if (status < 0) {
			note(NULL, ERROR, SYSTEM, "can't write event");
			goto end;
		}
	}

end:
	if (file != NULL)
		fclose(file);
	else if (new_fd != -1)
		close(new_fd);
}