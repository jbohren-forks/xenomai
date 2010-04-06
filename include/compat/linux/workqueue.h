/*
 * Copyright (C) 2005 Philippe Gerum <rpm@xenomai.org>.
 *
 * Xenomai is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * Emulates workqueues using task queues.
 */

#ifndef _XENO_COMPAT_LINUX_WORKQUEUE_H
#define _XENO_COMPAT_LINUX_WORKQUEUE_H

#include <linux/tqueue.h>

#define work_struct              tq_struct
#define schedule_work(x)         schedule_task(x)
#define flush_scheduled_work()   flush_scheduled_tasks()

/*
 * WARNING: This is not identical to 2.6 schedule_delayed_work as it delayes
 * the caller to schedule the task after the specified delay. That's fine for
 * our current use cases, though.
 */
#define schedule_delayed_work(work, delay) do {			\
	if (delay) {						\
		set_current_state(TASK_UNINTERRUPTIBLE);	\
		schedule_timeout(delay);			\
	}							\
	schedule_task(work);					\
} while (0)

#endif /* _XENO_COMPAT_LINUX_WORKQUEUE_H */
