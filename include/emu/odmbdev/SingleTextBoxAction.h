#ifndef __emu_odmbdev_SingleTextBoxAction_h__
#define __emu_odmbdev_SingleTextBoxAction_h__

#include "emu/odmbdev/Action.h"
#include "emu/odmbdev/Manager.h"

using namespace std;
using namespace emu::pc;

namespace emu { namespace odmbdev {
    class SingleTextBoxAction : public Action {
    public:
      SingleTextBoxAction(Crate * crate, Manager * manager, string box_content);

      void display(xgi::Output * out);
      void respond(xgi::Input * in, ostringstream & out);
    protected:
      string buttonLabel;
      string textBoxContent;
    };
  }
}

#endif
