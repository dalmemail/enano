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

#ifndef ENANO_EVENTS_H
#define ENANO_EVENTS_H

// There should be some compatibility with GNU nano:
// https://nano-editor.org/dist/latest/cheatsheet.html

enum {
	// File handling
	EVENT_SAVE_BUFFER=0,
	EVENT_SAVE_BUFFER_AS,
	EVENT_CLOSE_BUFFER,
	// Move cursor
	EVENT_SHOW_CURSOR,
	EVENT_HIDE_CURSOR,
	EVENT_MOVE_CURSOR_LEFT,
	EVENT_MOVE_CURSOR_RIGHT,
	EVENT_MOVE_CURSOR_UP,
	EVENT_MOVE_CURSOR_DOWN,

	EVENT_CHARACTER_ENTERED,

	EVENT_DELETE_KEY_ENTERED,
	// TODO: Check if we can rid of this one
	EVENT_VOID,
	NR_EVENTS
};

struct event {
	unsigned int event_type;
	// often this will be NULL
	void *additional_data;
};

enum {
	// TODO: Do we really need this success?
	EVENT_HANDLING_SUCCESS=0,
	ERROR_EVENT_NOT_FOUND,
	ERROR_OCCURRED_ERRNO_SET
};

// A struct result is the way the backends can return
// a (sometimes complex) result to the caller.
// This can be an error code, information requested by
// an event (such as the one triggered by pressing Ctrl+C)
// or whatever other thing
struct result {
	unsigned int result_type;
	// often this will be NULL too
	void *additional_data;
};

#endif /* ENANO_EVENTS_H */
