
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
  std::ifstream f;
  f.open(UI_LIBWIDGET_H);
  if (!f.is_open()) {
    std::cout << "Could not open \"" UI_LIBWIDGET_H "\"." << std::endl;
    return -1;
  }

  {
    bool gotTr2i18n = false;

    while (!f.eof()) {
      std::string output;
      getline(f, output);
      if (!gotTr2i18n) {
        gotTr2i18n = output.find("tr2i18n") != std::string::npos;
      }
      if (output.find("tr2xi18n") != std::string::npos) {
        std::cout << "ui_libwidget,h uses tr2xi18n, though it should not."
                  << std::endl;
        return -1;
      }
    }

    if (!gotTr2i18n) {
      std::cout << "Did not find tr2i18n in ui_libwidget.h" << std::endl;
      return -1;
    }
  }

  f.close();
  f.open(UI_MYWIDGET_H);
  if (!f.is_open()) {
    std::cout << "Could not open \"" UI_MYWIDGET_H "\"." << std::endl;
    return -1;
  }

  {
    bool gotTr2xi18n = false;

    while (!f.eof()) {
      std::string output;
      getline(f, output);
      if (!gotTr2xi18n) {
        gotTr2xi18n = output.find("tr2xi18n") != std::string::npos;
      }
      if (output.find("tr2i18n") != std::string::npos) {
        std::cout << "ui_mywidget,h uses tr2i18n, though it should not."
                  << std::endl;
        return -1;
      }
    }
    if (!gotTr2xi18n) {
      std::cout << "Did not find tr2xi18n in ui_mywidget.h" << std::endl;
      return -1;
    }
  }
  f.close();

  return 0;
}
