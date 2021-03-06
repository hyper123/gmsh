// Gmsh - Copyright (C) 1997-2014 C. Geuzaine, J.-F. Remacle
//
// See the LICENSE.txt file for license information. Please report all
// bugs and problems to the public mailing list <gmsh@geuz.org>.

#ifndef _GRAPHIC_WINDOW_H_
#define _GRAPHIC_WINDOW_H_

#include <string>
#include <vector>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Progress.H>
#if defined(__APPLE__)
#include <FL/Fl_Sys_Menu_Bar.H>
#endif
#include <FL/Fl_Menu_Bar.H>
#include "openglWindow.h"
#ifdef HAVE_ONELAB2
#include "onelab2Group.h"
#else
#include "onelabGroup.h"
#endif

class graphicWindow{
 private:
  std::string _title;
  bool _autoScrollMessages;
#if defined(__APPLE__)
  Fl_Sys_Menu_Bar *_sysbar;
#endif
  Fl_Menu_Bar *_bar;
  Fl_Tile *_tile;
  Fl_Window *_win, *_menuwin;
  Fl_Browser *_browser;
  onelabGroup *_onelab;
  Fl_Box *_bottom;
  Fl_Button *_butt[14];
  Fl_Progress *_label;
  int _minWidth, _minHeight;
 public:
  std::vector<openglWindow*> gl;
 public:
  graphicWindow(bool main=true, int numTiles=1, bool detachedMenu=false);
  ~graphicWindow();
  Fl_Window *getWindow(){ return _win; }
  Fl_Window *getMenuWindow(){ return _menuwin; }
  onelabGroup *getMenu(){ return _onelab; }
  Fl_Progress *getProgress(){ return _label; }
  Fl_Button *getSelectionButton(){ return _butt[9]; }
  int getMinWidth(){ return _minWidth; }
  int getMinHeight(){ return _minHeight; }
  void setAutoScroll(bool val){ _autoScrollMessages = val; }
  bool getAutoScroll(){ return _autoScrollMessages; }
  void setTitle(std::string str);
  void setStereo(bool st);
  int getGlWidth();
  void setGlWidth(int w);
  int getGlHeight();
  void setGlHeight(int h);
  int getMenuWidth();
  void setMenuWidth(int w);
  int getMenuHeight();
  int getMenuPositionX();
  int getMenuPositionY();
  void showMenu();
  void hideMenu();
  void showHideMenu();
  void detachMenu();
  void attachMenu();
  void attachDetachMenu();
  bool isMenuDetached(){ return _menuwin ? true : false; }
  bool split(openglWindow *g, char how);
  void setAnimButtons(int mode);
  void checkAnimButtons();
  int getMessageHeight();
  void setMessageHeight(int h);
  void showMessages();
  void hideMessages();
  void showHideMessages();
  void addMessage(const char *msg);
  void clearMessages();
  void saveMessages(const char *filename);
  void copySelectedMessagesToClipboard();
  void setMessageFontSize(int size);
  void changeMessageFontSize(int incr);
  void fillRecentHistoryMenu();
};

void file_quit_cb(Fl_Widget *w, void *data);
void file_watch_cb(Fl_Widget *w, void *data);
void mod_geometry_cb(Fl_Widget *w, void *data);
void mod_mesh_cb(Fl_Widget *w, void *data);
void mod_solver_cb(Fl_Widget *w, void *data);
void mod_post_cb(Fl_Widget *w, void *data);
void mod_back_cb(Fl_Widget *w, void *data);
void mod_forward_cb(Fl_Widget *w, void *data);
void geometry_reload_cb(Fl_Widget *w, void *data);
void mesh_1d_cb(Fl_Widget *w, void *data);
void mesh_2d_cb(Fl_Widget *w, void *data);
void mesh_3d_cb(Fl_Widget *w, void *data);
void help_about_cb(Fl_Widget *w, void *data);
void status_xyz1p_cb(Fl_Widget *w, void *data);
void status_options_cb(Fl_Widget *w, void *data);
void status_play_manual(int time, int incr, bool redraw=true);
void quick_access_cb(Fl_Widget *w, void *data);
void show_hide_message_cb(Fl_Widget *w, void *data);
void show_hide_menu_cb(Fl_Widget *w, void *data);
void attach_detach_menu_cb(Fl_Widget *w, void *data);

#endif
