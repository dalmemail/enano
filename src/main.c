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

#include <stdio.h>

#include <frontend/editor.h>

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("error\n");
		return 1;
	}
	/*initscr();
	start_color();
	cbreak();
	noecho();*/
	//wborder(stdscr, 0, 0, 0, 0, 0, 0, 0, 0);
	//init_pair(1, COLOR_BLACK, COLOR_WHITE);
	//wbkgd(stdscr, COLOR_PAIR(1));
	//for (char *p = str; *p; p++)
	//	waddch(stdscr, *p | COLOR_PAIR(1));
	//int c = getch();
	//if (c != ctrl('x'))
	//	getch();
	//run_editor();
	//endwin();
	//struct single_file_editor_data p;
	//return init_single_file_editor(&p, argv[1], 80, 80, 0, 0);
	run_editor(argv[1]);
	return 0;
}
