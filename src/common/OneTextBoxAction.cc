#include "emu/odmbdev/OneTextBoxAction.h"
#include "emu/odmbdev/Manager.h"

namespace emu { namespace odmbdev {
  OneTextBoxAction::OneTextBoxAction(Crate * crate, string buttonLabel)
    : Action(crate)
  {
    this->buttonLabel = buttonLabel;
    this->textBoxContent = string("15");
  }

  void OneTextBoxAction::display(xgi::Output * out){
    stringstream ss;
    ss << Manager::getSlotNumber();
    string str = ss.str();
    AddButtonWithTextBox(out,
				this->buttonLabel,
				"textbox",
				str);
  }
  // remember to call this base method with you override it, otherwise
  // textBoxContents will be empty!
  void OneTextBoxAction::respond(xgi::Input * in, ostringstream & out){
   this->textBoxContent = getFormValueString("textbox", in);
   Manager::setSlotNumber(atoi(this->textBoxContent.c_str()));
//    return atoi(this->textBoxContent.c_str());
  }
}
}
