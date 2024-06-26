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

#include <stdio.h>
#include <string.h>

#include <backend/single_buffer_editor.h>
#include <common/events.h>

#define ctrl(x)           ((x) & 0x1f)

// TODO: Put upper and lower bar in different files
static void draw_upper_bar(WINDOW *window)
{
        wbkgd(window, COLOR_PAIR(1));
	// TODO: Put some error checking in this functions
	// TODO: Check this: Bolding isn't working
	wattron(window, A_BOLD);
        mvwprintw(window, 0, 2, "(e)nano");
	wattroff(window, A_BOLD);
}

void run_editor(char *path)
{
	initscr();
	// raw() allows to use certain combinations like Control+S which
	// otherwise would raise a signal
	raw();
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
	int retval = editor.init(&editor, path, LINES - 1, COLS, 1, 0);
	if (retval < 0) {
		endwin();
		printf("Critical error at editor.init(): %s\n", strerror(-retval));
		return;
	}
	struct event reusable_event;
	struct result reusable_result;
	int exit = 0;
	editor.refresh_(&editor);
	// TODO: Use jump table here
	while (!exit) {
		reusable_event.event_type = EVENT_VOID;
		int c = wgetch(upper_bar_window);
		switch (c) {
			case ctrl('x'):
				exit = 1;
			break;
			case ctrl('s'):
				reusable_event.event_type = EVENT_SAVE_BUFFER;
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
		if (!exit) {
			editor.handle_event(&editor, &reusable_event, &reusable_result);
			editor.refresh_(&editor);
		}
	}
	editor.uninit(&editor);
	endwin();
}
