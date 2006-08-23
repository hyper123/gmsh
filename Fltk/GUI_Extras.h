#ifndef _GUI_EXTRAS_H_
#define _GUI_EXTRAS_H_

// Copyright (C) 1997-2006 C. Geuzaine, J.-F. Remacle
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
// 
// Please report all bugs and problems to <gmsh@geuz.org>.

int file_chooser(int multi, int create, const char *message,
		 const char *pat, char *fname=NULL);
char *file_chooser_get_name(int num);
int file_chooser_get_filter();
void file_chooser_get_position(int *x, int *y);

int arrow_editor(char *title, double &a, double &b, double &c);
int perspective_editor();

int jpeg_dialog(char *filename);
int gif_dialog(char *filename);
int generic_bitmap_dialog(char *filename, char *title, int format);
int gl2ps_dialog(char *filename, char *title, int format);
int options_dialog(char *filename);
int msh_dialog(char *filename);
int stl_dialog(char *filename);

#endif

