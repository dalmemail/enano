##
 # Copyright (C) 2024 Daniel Martin <dalmemail@gmail.com>
 #
 # This program is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, version 3 of the License.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ##

INC=-I./

all : main.o editor.o single_buffer_editor.o
	cc -Wall -o enano main.o editor.o single_buffer_editor.o -lncurses

main.o : main.c
	cc -Wall $(INC) -c main.c
editor.o : frontend/editor.c
	cc -Wall $(INC) -c frontend/editor.c
single_buffer_editor.o : backend/single_buffer_editor.c
	cc -Wall $(INC) -c backend/single_buffer_editor.c
clean :
	rm enano main.o editor.o single_buffer_editor.o
