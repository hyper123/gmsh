// $Id: GUI_Extras.cpp,v 1.28 2006-11-27 22:22:10 geuzaine Exp $
//
// Copyright (C) 1997-2007 C. Geuzaine, J.-F. Remacle
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

#include "Gmsh.h"
#include "GmshUI.h"
#include "GmshDefines.h"
#include "File_Picker.h"
#include "Shortcut_Window.h"
#include "CreateFile.h"
#include "Options.h"
#include "Context.h"
#include "Draw.h"

#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Menu_Window.H>
#include <errno.h>

extern Context_T CTX;

// File chooser

static File_Picker *fc = NULL;

int file_chooser(int multi, int create, const char *message,
		 const char *filter, char *fname)
{
  static char thefilter[1024] = "";
  static int thefilterindex = 0;

  Fl_File_Chooser::show_label = "Format:";
  Fl_File_Chooser::all_files_label = "All files (*)";

  if(strncmp(thefilter, filter, 1024)) {
    // reset the filter and the selection if the filter has changed
    strncpy(thefilter, filter, 1024);
    thefilterindex = 0;
  }

  if(!fc) {
    fc = new File_Picker(getenv("PWD") ? "." : CTX.home_dir, thefilter, 
			 Fl_File_Chooser::SINGLE, message);
    fc->position(CTX.file_chooser_position[0], CTX.file_chooser_position[1]);
  }

  if(multi)
    fc->type(Fl_File_Chooser::MULTI);
  else if(create)
    fc->type(Fl_File_Chooser::CREATE);
  else
    fc->type(Fl_File_Chooser::SINGLE);

  fc->label(message);
  fc->filter(thefilter);
  fc->filter_value(thefilterindex);
  if(fname)
    fc->value(fname);

  fc->show();

  while(fc->shown())
    Fl::wait();

  thefilterindex = fc->filter_value();

  if(fc->value())
    return fc->count();
  else
    return 0;
}

char *file_chooser_get_name(int num)
{
  if(!fc)
    return "";
  return (char *)fc->value(num);
}

int file_chooser_get_filter()
{
  if(!fc)
    return 0;
  return fc->filter_value();
}

void file_chooser_get_position(int *x, int *y)
{
  if(!fc)
    return;
  *x = fc->x();
  *y = fc->y();
}

// Arrow editor

int arrow_editor(char *title, double &a, double &b, double &c)
{
  struct _editor{
    Fl_Window *window;
    Fl_Value_Slider *sa, *sb, *sc;
    Fl_Button *apply, *cancel;
  };
  static _editor *editor = NULL;

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!editor){
    editor = new _editor;
    editor->window = new Dialog_Window(2 * BB + 3 * WB, 4 * BH + 3 * WB);
    editor->sa = new Fl_Value_Slider(WB, WB, BB, BH, "Head radius");
    editor->sa->type(FL_HOR_SLIDER);
    editor->sa->align(FL_ALIGN_RIGHT);
    editor->sb = new Fl_Value_Slider(WB, WB + BH, BB, BH, "Stem length");
    editor->sb->type(FL_HOR_SLIDER);
    editor->sb->align(FL_ALIGN_RIGHT);
    editor->sc = new Fl_Value_Slider(WB, WB + 2 * BH, BB, BH, "Stem radius");
    editor->sc->type(FL_HOR_SLIDER);
    editor->sc->align(FL_ALIGN_RIGHT);
    editor->apply = new Fl_Return_Button(WB, 2 * WB + 3 * BH, BB, BH, "Apply");
    editor->cancel = new Fl_Button(2 * WB + BB, 2 * WB + 3 * BH, BB, BH, "Cancel");
    editor->window->end();
    editor->window->hotspot(editor->window);
  }
  
  editor->window->label(title);
  editor->sa->value(a);
  editor->sb->value(b);
  editor->sc->value(c);
  editor->window->show();

  while(editor->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == editor->apply) {
	a = editor->sa->value();
	b = editor->sb->value();
	c = editor->sc->value();
	return 1;
      }
      if (o == editor->window || o == editor->cancel){
	editor->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// Perspective editor (aka z-clipping planes factor slider)

static void persp_change_factor(Fl_Widget* w, void* data){
  opt_general_clip_factor(0, GMSH_SET|GMSH_GUI, ((Fl_Slider*)w)->value());
  Draw();
}

class Release_Slider : public Fl_Slider {
  int handle(int event){ 
    switch (event) {
    case FL_RELEASE: 
      if(window())
	window()->hide();
      return 1;
    default:
      return Fl_Slider::handle(event);
    }
  };
public:
  Release_Slider(int x,int y,int w,int h,const char *l=0)
    : Fl_Slider(x, y, w, h, l) {}
};

int perspective_editor()
{
  struct _editor{
    Fl_Menu_Window *window;
    Release_Slider *sa;
  };
  static _editor *editor = NULL;

  if(!editor){
    editor = new _editor;
    editor->window = new Fl_Menu_Window(20, 100);
    editor->sa = new Release_Slider(0, 0, 20, 100);
    editor->sa->type(FL_VERT_NICE_SLIDER);
    editor->sa->minimum(12);
    editor->sa->maximum(0.75);
    editor->sa->callback(persp_change_factor);
    editor->window->border(0);
    editor->window->end();
  }

  editor->window->hotspot(editor->window);
  editor->sa->value(CTX.clip_factor);
  editor->window->show();
  return 0;
}

// save jpeg dialog

int jpeg_dialog(char *name)
{
  struct _jpeg_dialog{
    Fl_Window *window;
    Fl_Value_Slider *s[2];
    Fl_Check_Button *b;
    Fl_Button *ok, *cancel;
  };
  static _jpeg_dialog *dialog = NULL;

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _jpeg_dialog;
    int h = 3 * WB + 4 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h, "JPEG Options");
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->s[0] = new Fl_Value_Slider(WB, y, BB, BH, "Quality"); y += BH;
    dialog->s[0]->type(FL_HOR_SLIDER);
    dialog->s[0]->align(FL_ALIGN_RIGHT);
    dialog->s[0]->minimum(1);
    dialog->s[0]->maximum(100);
    dialog->s[0]->step(1);
    dialog->s[1] = new Fl_Value_Slider(WB, y, BB, BH, "Smoothing"); y += BH;
    dialog->s[1]->type(FL_HOR_SLIDER);
    dialog->s[1]->align(FL_ALIGN_RIGHT);
    dialog->s[1]->minimum(0);
    dialog->s[1]->maximum(100);
    dialog->s[1]->step(1);
    dialog->b = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Print text strings"); y += BH;
    dialog->b->type(FL_TOGGLE_BUTTON);
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->s[0]->value(CTX.print.jpeg_quality);
  dialog->s[1]->value(CTX.print.jpeg_smoothing);
  dialog->b->value(CTX.print.text);
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	opt_print_jpeg_quality(0, GMSH_SET | GMSH_GUI, (int)dialog->s[0]->value());
	opt_print_jpeg_smoothing(0, GMSH_SET | GMSH_GUI, (int)dialog->s[1]->value());
	opt_print_text(0, GMSH_SET | GMSH_GUI, (int)dialog->b->value());
	CreateOutputFile(name, FORMAT_JPEG);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save generic bitmap dialog

int generic_bitmap_dialog(char *name, char *title, int format)
{
  struct _generic_bitmap_dialog{
    Fl_Window *window;
    Fl_Check_Button *b;
    Fl_Button *ok, *cancel;
  };
  static _generic_bitmap_dialog *dialog = NULL;

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _generic_bitmap_dialog;
    int h = 3 * WB + 2 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h);
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->b = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Print text strings"); y += BH;
    dialog->b->type(FL_TOGGLE_BUTTON);
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->window->label(title);
  dialog->b->value(CTX.print.text);
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	opt_print_text(0, GMSH_SET | GMSH_GUI, (int)dialog->b->value());
	CreateOutputFile(name, format);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save gif dialog

int gif_dialog(char *name)
{
  struct _gif_dialog{
    Fl_Window *window;
    Fl_Check_Button *b[5];
    Fl_Button *ok, *cancel;
  };
  static _gif_dialog *dialog = NULL;

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _gif_dialog;
    int h = 3 * WB + 6 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h, "GIF Options");
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->b[0] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Dither"); y += BH;
    dialog->b[1] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Interlace"); y += BH;
    dialog->b[2] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Sort colormap"); y += BH;
    dialog->b[3] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Transparent background"); y += BH;
    dialog->b[4] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Print text strings"); y += BH;
    for(int i = 0; i < 5; i++){
      dialog->b[i]->type(FL_TOGGLE_BUTTON);
    }
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->b[0]->value(CTX.print.gif_dither);
  dialog->b[1]->value(CTX.print.gif_interlace);
  dialog->b[2]->value(CTX.print.gif_sort);
  dialog->b[3]->value(CTX.print.gif_transparent);
  dialog->b[4]->value(CTX.print.text);
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	opt_print_gif_dither(0, GMSH_SET | GMSH_GUI, dialog->b[0]->value());
	opt_print_gif_interlace(0, GMSH_SET | GMSH_GUI, dialog->b[1]->value());
	opt_print_gif_sort(0, GMSH_SET | GMSH_GUI, dialog->b[2]->value());
	opt_print_gif_transparent(0, GMSH_SET | GMSH_GUI, dialog->b[3]->value());
	opt_print_text(0, GMSH_SET | GMSH_GUI, dialog->b[4]->value());
	CreateOutputFile(name, FORMAT_GIF);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save ps/eps/pdf dialog

static void activate_gl2ps_choices(int format, int quality, Fl_Check_Button *b[5])
{
#if defined(HAVE_LIBZ)
  b[0]->activate();
#else
  b[0]->deactivate();
#endif
  switch(quality){
  case 0: // raster
    b[1]->deactivate(); 
    b[2]->deactivate(); 
    b[3]->deactivate(); 
    b[4]->deactivate();
    break;
  case 1: // simple sort
  case 3: // unsorted
    b[1]->activate();   
    b[2]->activate();
    b[3]->deactivate(); 
    if(format == FORMAT_PDF || format == FORMAT_SVG)
      b[4]->deactivate();
    else
      b[4]->activate();
    break;
  case 2: // bsp sort
    b[1]->activate();   
    b[2]->activate();
    b[3]->activate();   
    if(format == FORMAT_PDF || format == FORMAT_SVG)
      b[4]->deactivate();
    else
      b[4]->activate();
    break;
  }
}

int gl2ps_dialog(char *name, char *title, int format)
{
  struct _gl2ps_dialog{
    Fl_Window *window;
    Fl_Check_Button *b[6];
    Fl_Choice *c;
    Fl_Button *ok, *cancel;
  };
  static _gl2ps_dialog *dialog = NULL;

  static Fl_Menu_Item sortmenu[] = {
    {"Raster image", 0, 0, 0},
    {"Vector simple sort", 0, 0, 0},
    {"Vector accurate sort", 0, 0, 0},
    {"Vector unsorted", 0, 0, 0},
    {0}
  };

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _gl2ps_dialog;
    int h = 3 * WB + 8 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h);
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->c = new Fl_Choice(WB, y, BB + WB + BB / 2, BH, "Type"); y += BH;
    dialog->c->menu(sortmenu);
    dialog->c->align(FL_ALIGN_RIGHT);
    dialog->b[0] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Compress"); y += BH;
    dialog->b[1] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Print background"); y += BH;
    dialog->b[2] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Remove hidden primitives"); y += BH;
    dialog->b[3] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Optimize BSP tree"); y += BH;
    dialog->b[4] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Use level 3 shading"); y += BH;
    dialog->b[5] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Print text strings"); y += BH;
    for(int i = 0; i < 6; i++){
      dialog->b[i]->type(FL_TOGGLE_BUTTON);
    }
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->window->label(title);
  dialog->c->value(CTX.print.eps_quality);
  dialog->b[0]->value(CTX.print.eps_compress);
  dialog->b[1]->value(CTX.print.eps_background);
  dialog->b[2]->value(CTX.print.eps_occlusion_culling);
  dialog->b[3]->value(CTX.print.eps_best_root);
  dialog->b[4]->value(CTX.print.eps_ps3shading);
  dialog->b[5]->value(CTX.print.text);

  activate_gl2ps_choices(format, CTX.print.eps_quality, dialog->b);

  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;

      if (o == dialog->c){
	activate_gl2ps_choices(format, dialog->c->value(), dialog->b);
      }
      if (o == dialog->ok) {
	opt_print_eps_quality(0, GMSH_SET | GMSH_GUI, dialog->c->value());
	opt_print_eps_compress(0, GMSH_SET | GMSH_GUI, dialog->b[0]->value());
	opt_print_eps_background(0, GMSH_SET | GMSH_GUI, dialog->b[1]->value());
	opt_print_eps_occlusion_culling(0, GMSH_SET | GMSH_GUI, dialog->b[2]->value());
	opt_print_eps_best_root(0, GMSH_SET | GMSH_GUI, dialog->b[3]->value());
	opt_print_eps_ps3shading(0, GMSH_SET | GMSH_GUI, dialog->b[4]->value());
	opt_print_text(0, GMSH_SET | GMSH_GUI, dialog->b[5]->value());
	CreateOutputFile(name, format);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save options dialog

int options_dialog(char *name)
{
  struct _options_dialog{
    Fl_Window *window;
    Fl_Check_Button *b[2];
    Fl_Button *ok, *cancel;
  };
  static _options_dialog *dialog = NULL;

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _options_dialog;
    int h = 3 * WB + 3 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h, "Options");
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->b[0] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Save only modified options"); y += BH;
    dialog->b[0]->value(1);
    dialog->b[0]->type(FL_TOGGLE_BUTTON);
    dialog->b[1] = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Print help strings"); y += BH;
    dialog->b[1]->value(1);
    dialog->b[1]->type(FL_TOGGLE_BUTTON);
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	Print_Options(0, GMSH_FULLRC, dialog->b[0]->value(), dialog->b[1]->value(), name);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save msh dialog

int msh_dialog(char *name)
{
  struct _msh_dialog{
    Fl_Window *window;
    Fl_Check_Button *b;
    Fl_Choice *c;
    Fl_Button *ok, *cancel;
  };
  static _msh_dialog *dialog = NULL;

  static Fl_Menu_Item formatmenu[] = {
    {"Version 1.0", 0, 0, 0},
    {"Version 2.0 ASCII", 0, 0, 0},
    {"Version 2.0 Binary", 0, 0, 0},
    {0}
  };

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _msh_dialog;
    int h = 3 * WB + 3 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h, "MSH Options");
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->c = new Fl_Choice(WB, y, BB + BB / 2, BH, "Format"); y += BH;
    dialog->c->menu(formatmenu);
    dialog->c->align(FL_ALIGN_RIGHT);
    dialog->b = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Save all (ignore physicals)"); y += BH;
    dialog->b->type(FL_TOGGLE_BUTTON);
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->c->value((CTX.mesh.msh_file_version == 1.0) ? 0 : 
		   CTX.mesh.msh_binary ? 2 : 1);
  dialog->b->value(CTX.mesh.save_all);
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	opt_mesh_msh_file_version(0, GMSH_SET | GMSH_GUI, 
				  (dialog->c->value() == 0) ? 1. : 2.);
	opt_mesh_msh_binary(0, GMSH_SET | GMSH_GUI, 
			    (dialog->c->value() == 2) ? 1 : 0);
	opt_mesh_save_all(0, GMSH_SET | GMSH_GUI, dialog->b->value());
	CreateOutputFile(name, FORMAT_MSH);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save unv dialog

int unv_dialog(char *name)
{
  struct _unv_dialog{
    Fl_Window *window;
    Fl_Check_Button *b;
    Fl_Button *ok, *cancel;
  };
  static _unv_dialog *dialog = NULL;

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _unv_dialog;
    int h = 3 * WB + 2 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h, "UNV Options");
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->b = new Fl_Check_Button(WB, y, 2 * BB + WB, BH, "Save all (ignore physicals)"); y += BH;
    dialog->b->type(FL_TOGGLE_BUTTON);
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->b->value(CTX.mesh.save_all);
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	opt_mesh_save_all(0, GMSH_SET | GMSH_GUI, dialog->b->value());
	CreateOutputFile(name, FORMAT_UNV);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save bdf dialog

int bdf_dialog(char *name)
{
  struct _bdf_dialog{
    Fl_Window *window;
    Fl_Choice *c;
    Fl_Button *ok, *cancel;
  };
  static _bdf_dialog *dialog = NULL;

  static Fl_Menu_Item formatmenu[] = {
    {"Free field", 0, 0, 0},
    {"Small field", 0, 0, 0},
    {"Long field", 0, 0, 0},
    {0}
  };

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _bdf_dialog;
    int h = 3 * WB + 2 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h, "BDF Options");
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->c = new Fl_Choice(WB, y, BB + BB / 2, BH, "Format"); y += BH;
    dialog->c->menu(formatmenu);
    dialog->c->align(FL_ALIGN_RIGHT);
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->c->value(CTX.mesh.bdf_field_format);
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	opt_mesh_bdf_field_format(0, GMSH_SET | GMSH_GUI, dialog->c->value());
	CreateOutputFile(name, FORMAT_BDF);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}

// save stl dialog

int stl_dialog(char *name)
{
  struct _stl_dialog{
    Fl_Window *window;
    Fl_Choice *c;
    Fl_Button *ok, *cancel;
  };
  static _stl_dialog *dialog = NULL;

  static Fl_Menu_Item formatmenu[] = {
    {"ASCII", 0, 0, 0},
    {"Binary", 0, 0, 0},
    {0}
  };

  const int BH = 2 * CTX.fontsize + 1;
  const int BB = 7 * CTX.fontsize;
  const int WB = 7;

  if(!dialog){
    dialog = new _stl_dialog;
    int h = 3 * WB + 2 * BH, w = 2 * BB + 3 * WB, y = WB;
    // not a "Dialog_Window" since it is modal 
    dialog->window = new Fl_Double_Window(w, h, "STL Options");
    dialog->window->box(GMSH_WINDOW_BOX);
    dialog->c = new Fl_Choice(WB, y, BB + BB / 2, BH, "Format"); y += BH;
    dialog->c->menu(formatmenu);
    dialog->c->align(FL_ALIGN_RIGHT);
    dialog->ok = new Fl_Return_Button(WB, y + WB, BB, BH, "OK");
    dialog->cancel = new Fl_Button(2 * WB + BB, y + WB, BB, BH, "Cancel");
    dialog->window->set_modal();
    dialog->window->end();
    dialog->window->hotspot(dialog->window);
  }
  
  dialog->c->value(CTX.mesh.stl_binary ? 1 : 0);
  dialog->window->show();

  while(dialog->window->shown()){
    Fl::wait();
    for (;;) {
      Fl_Widget* o = Fl::readqueue();
      if (!o) break;
      if (o == dialog->ok) {
	opt_mesh_stl_binary(0, GMSH_SET | GMSH_GUI, dialog->c->value());
	CreateOutputFile(name, FORMAT_STL);
	dialog->window->hide();
	return 1;
      }
      if (o == dialog->window || o == dialog->cancel){
	dialog->window->hide();
	return 0;
      }
    }
  }
  return 0;
}
