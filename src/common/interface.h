/*
 * Copyright (C) 2024 Daniel Martin <dalmemail@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ENANO_INTERFACE_H
#define ENANO_INTERFACE_H

#include <common/events.h>

/*
 * This is essentially OOP: Here we define an interface every
 * "editor" (backend) has to comply with. Every editor must declare
 * and fill an struct editor_object (this would be the "definition" of the
 * editor class). Instantiate objects of the class can be
 * done by making copies of said struct + call init().
 */
struct editor_object {
	void *data;
	int (*init)(struct editor_object *, const char *, int, int, int, int);
	void (*uninit)(struct editor_object *);
	int (*handle_event)(struct editor_object *, struct event *);
	// ncurses defines a macro called refresh(), so we can't use that name
	void (*refresh_)(struct editor_object *);
};

#endif /* ENANO_INTERFACE_H */
