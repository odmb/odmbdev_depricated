#ifndef __emu_odmbdev_ThreeTextBoxAction_h__
#define __emu_odmbdev_ThreeTextBoxAction_h__

#include "emu/odmbdev/Action.h"

using namespace std;
using namespace emu::pc;

namespace emu { namespace odmbdev {
    class ThreeTextBoxAction : public Action {
    public:
      ThreeTextBoxAction(Crate * crate, string buttonLabel);

      void display(xgi::Output * out);
      void respond(xgi::Input * in, ostringstream & out);
    protected:
      string buttonLabel;
      string textBoxContent1;
      string textBoxContent2;
      string textBoxContent3;
    };
  }
}

#endif
