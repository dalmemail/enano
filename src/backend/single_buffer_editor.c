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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <backend/single_buffer_editor.h>
#include <common/events.h>
#include <curses.h>

#define SPACES_IN_A_TAB 8

#define max(x,y) (x >= y) ? x : y

struct line {
	size_t size;
	// TODO: We don't use this yet !!
	// length, as it would be returned by strlen()
	size_t length;
	// TODO: Represent each line as a linked list too?
	// That would allow O(1) insertion too, although jumping would become O(n) (currently O(1))
	// '\0' terminated string
	char *line_str;
};

struct line_linked_list_node {
	struct line line;
	struct line_linked_list_node *next;
	struct line_linked_list_node *prev;
};

struct single_buffer_editor_data {
	WINDOW *window;
	size_t window_nlines;
	size_t window_ncols;
	// (x0, y0) is the position of the upper left corner of the window
	// in the full screen
	// TODO: Check if we really need this
	size_t window_pos_x0;
	size_t window_pos_y0;

	FILE *file_descriptor;
	// the real buffer we use, the file it's split into lines
	// Linked list allows inserting lines in O(1), jumping to a line in O(n)
	struct line_linked_list_node *lines;

	size_t n_lines;

	// current position on the file
	// this is NOT the position of the cursor in the screen
	size_t pos_x;
	size_t pos_y;
	struct line_linked_list_node *line_y;
	char show_cursor;
	// some operations might need triggering a full editor window
	// rewrite
	char clear_window;

	// holds the line at the top of the window
	struct line_linked_list_node *top_print_line;
	size_t top_print_line_y;
};

// Auxiliary functions go here

static unsigned int count_lines(char *buf, size_t buf_size)
{
	size_t ret = 0;
	for (size_t i = 0; i < buf_size; i++)
		if (buf[i] == '\n')
			ret++;

	return ret;
}

static size_t count_tabs(char *s, size_t limit)
{
	size_t ret = 0;
	for (int i = 0; i < limit; i++)
		if (s[i] == '\t')
			ret++;

	return ret;
}

// return the length of the line, using '\n' as End-Of-Line mark
static size_t line_length(char *line)
{
	char *start = line;
	for (; *line != '\n'; line++);
	return line - start;
}

static void split_in_lines(char *buf, size_t n_lines, struct line_linked_list_node *lines)
{
	struct line_linked_list_node *current_node = lines;
	current_node->prev = NULL;
	struct line_linked_list_node *next_node;
	for (size_t i = 0; i < n_lines; i++) {
		size_t line_to_copy_length = line_length(buf);
		// TODO: Choose minimum between 2*length and 32, 64 or 128
		current_node->line.size = line_to_copy_length * 2;
		if (current_node->line.size == 0)
			// TODO: Don't hardcode this
			current_node->line.size = 64;

		current_node->line.length = line_to_copy_length;
		current_node->line.line_str = (char *)malloc(current_node->line.size * sizeof(char));
		// TODO: Error handling here !!
		buf[line_to_copy_length] = '\0';
		snprintf(current_node->line.line_str, current_node->line.size, "%s", buf);
		// TODO: Error handling here ?
		buf += line_to_copy_length + 1;

		next_node = (struct line_linked_list_node *)malloc(sizeof(struct line_linked_list_node));
		// TODO: Error handling here !!
		next_node->prev = current_node;
		current_node->next = next_node;
		current_node = next_node;
	}
	next_node->next = NULL;
	next_node->line.size = 0;
	next_node->line.length = 0;
}

static struct line_linked_list_node *alloc_linked_list_node(size_t size)
{
	struct line_linked_list_node *ret =
		(struct line_linked_list_node *)malloc(sizeof(struct line_linked_list_node));

	if (ret == NULL) {
		// TODO: Critical bug, but this is not acceptable behavior
		exit(1);
	}

	ret->line.line_str = (char *)malloc(size * sizeof(char));
	if (ret->line.line_str == NULL) {
		// TODO: Critical bug, but this is not acceptable behavior
		exit(1);
	}

	ret->line.size = size;
	ret->line.line_str[0] = '\0';
	ret->line.length = 0;

	return ret;
}

static void free_linked_list_node(struct line_linked_list_node *node)
{
	free(node->line.line_str);
	free(node);
}

static void resize_line_buffer(struct line *line)
{
	line->size *= 2;
	line->line_str = realloc(line->line_str, line->size);
	if (line->line_str == NULL) {
		// TODO: Critical failure. Handle in another way
		exit(1);
	}
}

static void move_str_right_1_char(char *begin, char *end)
{
	for (; end != begin; end--)
		*end = *(end - 1);
}

static void move_str_left_1_char(char *begin, char *end)
{
	for (; begin != end; begin++)
		*begin = *(begin + 1);
}

static void concat_lines(struct line *dst, struct line *src)
{
	// we need at least dst->length + src->length + 1 ('\0' char) bytes
	if (dst->length + src->length >= dst->size) {
		dst->size += src->length;
		dst->size += 2;
		dst->line_str = realloc(dst->line_str, dst->size);
		// TODO: A function for handling all the realloc stuff
		if (dst->line_str == NULL) {
			// TODO: Handle this in a more acceptable way
			exit(1);
		}
	}

	strncpy(&dst->line_str[dst->length], src->line_str, src->length + 1);
	dst->length += src->length;
}

//----------------------------------------------------------------------------------------//

// Functions that implement editor capabilities: Like moving the cursor, copy, paste, ....

static void show_cursor(struct single_buffer_editor_data *p)
{
	p->show_cursor = 1;
}

static void hide_cursor(struct single_buffer_editor_data *p)
{
	p->show_cursor = 0;
}

static void move_cursor_left(struct single_buffer_editor_data *p)
{
	if (p->pos_x > 0)
		p->pos_x--;
	else if (p->pos_x == 0 && p->pos_y > 0) {
		p->pos_y--;
		p->line_y = p->line_y->prev;
		p->pos_x = p->line_y->line.length;
	}
}

static void move_cursor_right(struct single_buffer_editor_data *p)
{
	if (p->pos_x < p->line_y->line.length)
		p->pos_x++;
	else if (p->pos_x == p->line_y->line.length && p->pos_y < p->n_lines) {
		p->pos_x = 0;
		p->pos_y++;
		p->line_y = p->line_y->next;
	}
}

static void move_cursor_up(struct single_buffer_editor_data *p)
{
	if (p->pos_y > 0) {
		p->pos_y--;
		p->line_y = p->line_y->prev;
		p->pos_x = (p->line_y->line.length >= p->pos_x) ? p->pos_x : p->line_y->line.length;
	}
}

static void move_cursor_down(struct single_buffer_editor_data *p)
{
	if (p->pos_y < p->n_lines) {
		p->pos_y++;
		p->line_y = p->line_y->next;
		p->pos_x = (p->line_y->line.length >= p->pos_x) ? p->pos_x : p->line_y->line.length;
	}
}

// TODO: This could be further optimized if when p->pos_x == 0 we just
// place a new line before the current line
static void insert_new_line(struct single_buffer_editor_data *p)
{
	struct line_linked_list_node *current_line = p->line_y;
	// TODO: Put '64' in a constant
	size_t new_line_size = max(current_line->line.length - p->pos_x + 1, 64);
	struct line_linked_list_node *new_line = alloc_linked_list_node(new_line_size);
	new_line->next = current_line->next;
	new_line->prev = current_line;
	current_line->next = new_line;
	if (new_line->next != NULL)
		new_line->next->prev = new_line;

	if (p->pos_x != current_line->line.length) {
		strncpy(new_line->line.line_str,
			&current_line->line.line_str[p->pos_x], new_line->line.size);

		new_line->line.length = current_line->line.length - p->pos_x;
		current_line->line.line_str[p->pos_x] = '\0';
		current_line->line.length = p->pos_x;
	}

	p->pos_x = 0;
	p->pos_y++;
	p->n_lines++;
	p->line_y = new_line;

	p->clear_window = 1;
}

static void remove_current_character(struct single_buffer_editor_data *p)
{
	struct line_linked_list_node *current_line = p->line_y;
	struct line_linked_list_node *prev_line = p->line_y->prev;
	if (p->pos_x == 0) {
		if (p->pos_y == 0)
			return;

		size_t prev_line_length = prev_line->line.length;
		concat_lines(&prev_line->line, &current_line->line);
		prev_line->next = current_line->next;
		if (prev_line->next != NULL)
			prev_line->next->prev = prev_line;

		free_linked_list_node(current_line);
		p->n_lines--;
		p->pos_y--;
		p->pos_x = prev_line_length;
		p->line_y = prev_line;
	}
	else {
		// if (p->pos_x < current_line->line.length)
		move_str_left_1_char(
			&current_line->line.line_str[p->pos_x - 1],
			&current_line->line.line_str[current_line->line.length]);
		p->pos_x--;
		current_line->line.length--;
		current_line->line.line_str[current_line->line.length] = '\0';
	}

	// TODO: Right now we're triggering a full window rewrite everytime
	// the user removes something. It'll be probably more efficient (although
	// it's not sure, testing is needed) to have a clear_window flag per line
	// and fully rewrite those lines by printing '\0' to fill the gap between
	// the line new end and window_ncols characters that fit on the screen line
	p->clear_window = 1;
}

// *c MUST be different from '\n'
static void put_character(struct single_buffer_editor_data *p, char *c)
{
	struct line *line = &p->line_y->line;
	// make sure there is space available
	if (line->length + 1 >= line->size)
		resize_line_buffer(line);

	// are we inserting at the end of the line?
	if (line->length == p->pos_x) {
		line->line_str[p->pos_x] = *c;
		p->pos_x++;
		line->line_str[p->pos_x] = '\0';
		line->length++;
	}
	else {
		move_str_right_1_char(
			&line->line_str[p->pos_x], &line->line_str[line->length + 1]);
		line->line_str[p->pos_x] = *c;
		p->pos_x++;
		line->length++;
	}
}

//---------------------------------------------------------------------------------------//

// Functions that implement the editor_object interface defined at common/interface.h

static int init_single_buffer_editor(struct editor_object *self, const char *path, int nlines, int ncols, int y, int x)
{
	struct single_buffer_editor_data *p = (struct single_buffer_editor_data *)malloc(sizeof(struct single_buffer_editor_data));
	if (p == NULL)
		return EINVAL;

	self->data = (void *)p;

	p->window = newwin(nlines, ncols, y, x);
	// TODO: Improve this
	if (!p->window) {
		return EFAULT;
	}

	p->window_nlines = nlines;
	p->window_ncols = ncols;
	p->window_pos_x0 = x;
	p->window_pos_y0 = y;
	p->file_descriptor = fopen(path, "r");
	if (!p->file_descriptor) {
		delwin(p->window);
		return errno;
	}

	struct stat st;
	if (stat(path, &st) < 0) {
		delwin(p->window);
		fclose(p->file_descriptor);
		return errno;
	}

	char *buffer;
	size_t buffer_size;
	buffer_size = st.st_size;
	buffer = (char *)malloc(buffer_size * sizeof(char));
	if (buffer == NULL) {
		delwin(p->window);
		fclose(p->file_descriptor);
		return errno;
	}

	if (fread(buffer, buffer_size, 1, p->file_descriptor) != 1) {
		delwin(p->window);
		fclose(p->file_descriptor);
		free(buffer);
		// TODO: put here another error code
		// DONT RETURN -1: You have to return something in the 0..255 range
		return -1;
	}

	p->n_lines = count_lines(buffer, buffer_size);
	p->lines = (struct line_linked_list_node *)malloc(sizeof(struct line_linked_list_node));
	if (p->lines == NULL) {
		delwin(p->window);
		fclose(p->file_descriptor);
		free(buffer);
		return errno;
	}

	split_in_lines(buffer, p->n_lines, p->lines);
	free(buffer);

	p->pos_x = 0;
	p->pos_y = 0;
	p->line_y = p->lines;
	p->show_cursor = 1;
	p->clear_window = 0;

	p->top_print_line = p->lines;
	p->top_print_line_y = 0;

	//TODO: Checks this
	return 0;
}

static void uninit_single_buffer_editor(struct editor_object *self)
{
	struct single_buffer_editor_data *p = (struct single_buffer_editor_data *)self->data;

	delwin(p->window);
	struct line_linked_list_node *current_node = p->lines;
	for (; current_node->next != NULL; current_node = current_node->next) {
		current_node = current_node->next;
		free_linked_list_node(current_node->prev);
	}
	free_linked_list_node(current_node);
	fclose(p->file_descriptor);
}

// TODO: split switch in functions
static int single_buffer_editor_handle_event(struct editor_object *self, struct event *event)
{
	struct single_buffer_editor_data *p = (struct single_buffer_editor_data *)self->data;
	// TODO: Better name for this
	char user_entered_character;

	//TODO: Use a jump table here ??
	switch (event->event_type) {
		case EVENT_SHOW_CURSOR:
			show_cursor(p);
		break;
		case EVENT_HIDE_CURSOR:
			hide_cursor(p);
		break;
		case EVENT_MOVE_CURSOR_LEFT:
			move_cursor_left(p);
		break;
		case EVENT_MOVE_CURSOR_RIGHT:
			move_cursor_right(p);
		break;
		case EVENT_MOVE_CURSOR_UP:
			move_cursor_up(p);
		break;
		case EVENT_MOVE_CURSOR_DOWN:
			move_cursor_down(p);
		break;
		case EVENT_DELETE_KEY_ENTERED:
			remove_current_character(p);
		break;
		case EVENT_CHARACTER_ENTERED:
			user_entered_character = *((char *)event->additional_data);
			if (user_entered_character == '\n')
				insert_new_line(p);
			else
				put_character(p, (char *)event->additional_data);
		break;
		default:
			return ERROR_EVENT_NOT_FOUND;
	}
	return EVENT_HANDLING_SUCCESS;
}

// TODO: Don't refresh the full window all the time
// Sometimes we're just moving the cursor, etc and we
// don't need to refresh it all??
static void single_buffer_editor_refresh(struct editor_object *self)
{
	struct single_buffer_editor_data *p = (struct single_buffer_editor_data *)self->data;

	char top_has_changed = 0;
	// calculate the new top_print_line
	if (p->pos_y < p->top_print_line_y) {
		p->top_print_line_y = p->pos_y;
		p->top_print_line = p->line_y;
		top_has_changed = 1;
	}
	// TODO: Do scroll here better, since we should only move down 1 line at a time
	else if (p->pos_y >= p->top_print_line_y + p->window_nlines) {
		// On practice this will only iterate 1 time
		while (p->pos_y >= p->top_print_line_y + p->window_nlines) {
			p->top_print_line_y++;
			p->top_print_line = p->top_print_line->next;
			top_has_changed = 1;
		}
	}

	if (top_has_changed || p->clear_window) {
		wclear(p->window);
		if (p->clear_window)
			p->clear_window = 0;
	}

	struct line_linked_list_node *current_line = p->top_print_line;
	for (int i = 0; i < p->window_nlines && current_line != NULL; i++) {
		// TODO: Take tabs in consideration when writing lines
		mvwaddnstr(p->window, i, 0, current_line->line.line_str,
			(current_line->line.length > p->window_ncols) ? p->window_ncols : current_line->line.length);

		current_line = current_line->next;
	}

	mvwprintw(p->window, 0, 0, "%d %d", p->pos_x, p->pos_y);
	// TODO: Use a handmade cursor
	if (p->show_cursor) {
		size_t n_tabs = count_tabs(p->line_y->line.line_str, p->pos_x);
		size_t cursor_x = (p->pos_x + n_tabs*(SPACES_IN_A_TAB - 1)) % p->window_ncols;
		size_t cursor_y = p->pos_y - p->top_print_line_y;
		mvwprintw(p->window, 1, 0, " %d ", cursor_y);
		wmove(p->window, cursor_y, cursor_x);
	}

	wrefresh(p->window);
}

struct editor_object single_buffer_editor_object = {
        .data = NULL,
        .init = init_single_buffer_editor,
        .uninit = uninit_single_buffer_editor,
        .handle_event = single_buffer_editor_handle_event,
        .refresh_ = single_buffer_editor_refresh
};
