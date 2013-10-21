#include "emu/odmbdev/SingleTextBoxAction.h"
#include "emu/odmbdev/Manager.h"

namespace emu { namespace odmbdev {
  SingleTextBoxAction::SingleTextBoxAction(Crate * crate, emu::odmbdev::Manager * manager, string buttonLabel)
    : Action(crate, manager)
  {
    this->buttonLabel = buttonLabel;
    this->textBoxContent = string("");
  }

  void SingleTextBoxAction::display(xgi::Output * out){
    addButtonWithTextBox(out,
			 this->buttonLabel,
			 "textbox",
			 "");
  }
  // remember to call this base method with you override it, otherwise
  // textBoxContents will be empty!
  void SingleTextBoxAction::respond(xgi::Input * in, ostringstream & out){
   this->textBoxContent = getFormValueString("textbox", in);
  }
}
}
