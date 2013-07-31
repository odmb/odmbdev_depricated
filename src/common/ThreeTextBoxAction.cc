#include "emu/odmbdev/ThreeTextBoxAction.h"

namespace emu { namespace odmbdev {
  ThreeTextBoxAction::ThreeTextBoxAction(Crate * crate, string buttonLabel)
    : Action(crate)
  {
    this->buttonLabel = buttonLabel;
    this->textBoxContent1 = string("");
    this->textBoxContent2 = string("");
    this->textBoxContent3 = string("");
  }

  void ThreeTextBoxAction::display(xgi::Output * out){
    AddButtonWithThreeTextBoxes(out,
				this->buttonLabel,
				"textbox1",
				"1",
				"textbox2",
				"",
				"textbox3",
				"");
  }
  // remember to call this base method with you override it, otherwise
  // textBoxContents will be empty!
 void ThreeTextBoxAction::respond(xgi::Input * in, ostringstream & out){
    this->textBoxContent1 = getFormValueString("textbox1", in);
    this->textBoxContent2 = getFormValueString("textbox2", in);
    this->textBoxContent3 = getFormValueString("textbox3", in);
  }
}
}
