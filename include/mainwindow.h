#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#ifdef _WIN32
#define _WINSOCKAPI_
#define NOMINMAX_
#endif

#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Chart.H>

#include <string>

class mainwindow : public Fl_Window
{
public:
  mainwindow();
  ~mainwindow();

  Fl_Input  *taddInput;
  Fl_Button *refreshButton;
  Fl_Box *minerBox;
  Fl_Box *totalSolsBox;
  Fl_Box *networkSolsBox;
  Fl_Box *immaturebalanceBox;
  Fl_Box *balanceBox;
  Fl_Box *paidBox;
  Fl_Chart *hashrateChart;
private:
  bool conf_exists = false;
  static void onRefreshButtonPressed(Fl_Widget*, void*);
  void checkRead();
  void update(std::string tadd);
  void create_or_modify_config(std::string tadd);
  const std::string address = "https://hush.miningspeed.com";
};

#endif // MAINWINDOW_H
