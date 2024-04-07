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

#include <curses.h>

#include <backend/single_buffer_editor.h>
#include <common/events.h>

#define ctrl(x)           ((x) & 0x1f)

static void draw_upper_bar(WINDOW *window)
{
        wbkgd(window, COLOR_PAIR(1));
	// TODO: Put some error checking in this functions
	wattron(window, A_BOLD);
        mvwprintw(window, 0, 2, "(e)nano");
	wattroff(window, A_BOLD);
}

void run_editor(char *path)
{
	initscr();
	start_color();
	cbreak();
	noecho();
	WINDOW *upper_bar_window = newwin(1, COLS, 0, 0);
	// for bright white color :)
	wattron(upper_bar_window, A_BOLD);
	// for capturing special keys in wgetch()
	keypad(upper_bar_window, TRUE);
	init_pair(1, COLOR_BLACK, COLOR_WHITE);
	draw_upper_bar(upper_bar_window);
	wrefresh(upper_bar_window);
	struct editor_object editor = single_buffer_editor_object;
	if (editor.init(&editor, path, LINES - 1, COLS, 1, 0) != 0) {
		goto out;
	}
	struct event reusable_event;
	int exit = 0;
	editor.refresh_(&editor);
	while (!exit) {
		reusable_event.event_type = EVENT_VOID;
		int c = wgetch(upper_bar_window);
		switch (c) {
			case ctrl('x'):
				exit = 1;
			break;
			case KEY_UP:
				reusable_event.event_type = EVENT_MOVE_CURSOR_UP;
			break;
			case KEY_DOWN:
				reusable_event.event_type = EVENT_MOVE_CURSOR_DOWN;
			break;
			case KEY_LEFT:
				reusable_event.event_type = EVENT_MOVE_CURSOR_LEFT;
			break;
			case KEY_RIGHT:
				reusable_event.event_type = EVENT_MOVE_CURSOR_RIGHT;
			break;
			case KEY_BACKSPACE:
			case KEY_DC:
				reusable_event.event_type = EVENT_DELETE_KEY_ENTERED;
			break;
			default:
				// TODO: Constants for this
				if (0 <= c && c <= 255) {
					reusable_event.event_type = EVENT_CHARACTER_ENTERED;
					reusable_event.additional_data = (void *)&c;
				}
		}
		editor.handle_event(&editor, &reusable_event);
		editor.refresh_(&editor);
	}
out:	endwin();
}
