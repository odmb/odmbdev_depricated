#include "emu/odmbdev/Action.h"

using namespace cgicc;

namespace emu { namespace odmbdev {

  Action::Action(Crate * crate) {
    this->crate = crate;

    ccb  = this->crate->ccb();
    ddus = this->crate->ddus();
    tmb  = this->crate->tmbs().at(0);
    dmbs = this->crate->daqmbs();
  }

    Action::Action(Crate * crate, Manager* manager):
      crate(crate),
      dmbs(crate->daqmbs()),
      ddus(crate->ddus()),
      tmb(crate->tmbs().at(0)),
      ccb(crate->ccb()),
      alct(tmb->alctController()),
      manager(manager)
    {}

  int Action::getFormValueInt(const string form_element, xgi::Input *in)
  {
    const cgicc::Cgicc cgi(in);
    int form_value;
    cgicc::const_form_iterator name = cgi.getElement(form_element);
    if (name !=cgi.getElements().end())
      {
	form_value = cgi[form_element]->getIntegerValue();
      }
    else
      {
	XCEPT_RAISE( xcept::Exception, "Form element, " + form_element + ", was not found." );
      }
    return form_value;
  }

  int Action::getFormValueIntHex(const string form_element, xgi::Input *in)
  {
    const cgicc::Cgicc cgi(in);
    int form_value;
    cgicc::const_form_iterator name = cgi.getElement(form_element);
    if (name != cgi.getElements().end())
      {
	stringstream convertor;
	string hex_as_string = cgi[form_element]->getValue();
	convertor << hex << hex_as_string;
	convertor >> form_value; 
      }
    else
      {
	XCEPT_RAISE( xcept::Exception, "Form element, " + form_element + ", was not found." );
      }
    return form_value;
  }


  float Action::getFormValueFloat(const string form_element, xgi::Input *in)
  {
    const cgicc::Cgicc cgi(in);
    float form_value;
    cgicc::const_form_iterator name = cgi.getElement(form_element);
    if (name != cgi.getElements().end())
      {
	form_value = cgi[form_element]->getDoubleValue();
      }
    else
      {
	XCEPT_RAISE( xcept::Exception, "Form element, " + form_element + ", was not found." );
      }
    return form_value;
  }

  string Action::getFormValueString(const string form_element, xgi::Input *in)
  {
    const cgicc::Cgicc cgi(in);
    string form_value;
    cgicc::const_form_iterator name = cgi.getElement(form_element);
    if (name != cgi.getElements().end())
      {
	form_value = cgi[form_element]->getValue();
      }
    else
      {
	XCEPT_RAISE( xcept::Exception, "Form element, " + form_element + ", was not found." );
      }
    return form_value;
  }
        
  //         void Action::getFormValueFile(const string form_element, xgi::Input *in, std::ostream& outstream)
  //         {
  //             const cgicc::Cgicc cgi(in);
  //             string form_value;
  //             cgicc::const_file_iterator name = cgi.getFile(form_element);
  //             if (name != cgi.getFiles().end())
  //             {                
  //                 name->writeToStream(outstream);
  //             }
  //             else
  //             {
  //                 XCEPT_RAISE( xcept::Exception, "Form element, " + form_element + ", was not found.");
  //             }

  //             return;
  //         }

  void Action::AddButton(xgi::Output *out,
			 const string button_name)
    throw (xgi::exception::Exception)
  {
    *out << cgicc::input().set("type","submit")
      .set("value",button_name)
	 << endl;
  }

  void Action::AddButtonWithTextBox(xgi::Output *out,
				    const string button_name,
				    const string textboxname,
				    const string textbox_default_value)
    throw (xgi::exception::Exception)
  {
    *out << "Slot Number: " << cgicc::input().set("type","text")
      .set("value",textbox_default_value)
      .set("name",textboxname) << cgicc::input().set("type","submit").set("value",button_name) << endl;
  }

  void Action::AddButtonWithTwoTextBoxes(xgi::Output *out,
					 const string button_name,
					 const string textboxname1,
					 const string textbox_default_value1,
					 const string textboxname2,
					 const string textbox_default_value2)
    throw (xgi::exception::Exception)
  {
    *out << cgicc::input().set("type","submit")
      .set("value",button_name)
	 << endl
	 << cgicc::input().set("type","text")
      .set("value",textbox_default_value1)
      .set("name",textboxname1)
	 << cgicc::input().set("type","text")
      .set("value",textbox_default_value2)
      .set("name",textboxname2);
  }

  void Action::AddButtonWithLongTextBox(xgi::Output *out,
					const string button_name,
					const string textboxname,
					const string textbox_default_value)
    throw (xgi::exception::Exception)
  {
    *out << cgicc::div().set("style",
			     // we need a std::string to use `+'
			     string("border: #000 solid 1px; ")
			     + "padding: 1em; ")
	 << cgicc::input().set("type","submit")
      .set("value",button_name)
	 << endl << cgicc::br() << endl
	 << cgicc::textarea().set("style",
				  string("width: 100%; ")
				  + "margin-top: 1em; "
				  + "height: 10em; ")
      .set("name",textboxname)
	 << textbox_default_value
	 << cgicc::textarea()
	 << cgicc::div();
  }

  void Action::AddButtonWithFourTextBoxes(xgi::Output *out,
                                           const string button_name,
                                           const string textboxname1,
                                           const string textbox_default_value1,
                                           const string textboxname2,
                                           const string textbox_default_value2,
                                           const string textboxname3,
                                           const string textbox_default_value3,
                                           const string textboxname4,
                                           const string textbox_default_value4)
    throw (xgi::exception::Exception){
    *out << cgicc::input().set("type","submit")
      .set("value",button_name)
         << endl  
	 << cgicc::input().set("type","text").set("style",std::string("width: 40px; "))
      .set("value",textbox_default_value1)
      .set("name",textboxname1) << " times" 
	 << cgicc::br() << " File: "
         << cgicc::input().set("type","text").set("style",std::string("width: 468px; margin-top: 1em; "))
      .set("value",textbox_default_value2)
      .set("name",textboxname2)
         << cgicc::br() << "Box Number: "
         << cgicc::input().set("type","text").set("style",std::string("width: 50px; margin-top: 1em; "))
      .set("value",textbox_default_value3)
      .set("name",textboxname3)
         << endl << cgicc::br() << endl

         << cgicc::textarea().set("style",
                                  std::string("width: 515px; ")
                                  + "margin-top: 1em; "
                                  + "height: 500px; ")
      .set("name", textboxname4)
         << textbox_default_value4
         << cgicc::textarea()
	 << endl;
    //         << cgicc::div();
  }
  
  // static void AddButtonWithFileUpload(xgi::Output *out,
  //                                     const string button_name)
  //     throw (xgi::exception::Exception)
  // {
  //     *out << "<form method=\"post\" action=\""
  //          << 
  // }
} // end namespace odmbdev
} // end namespace emu

/* Tom is going to be fucking with this code.  Here's the working version:

    throw (xgi::exception::Exception){
    *out << cgicc::div().set("style",
                             std::string("border: #000 solid 1px; ")
                             + "padding: 1em; ")
         << cgicc::input().set("type","submit")
      .set("value",button_name)
         << endl << " Repeat n times: "
         << cgicc::input().set("type","text")
      .set("value",textbox_default_value1)
      .set("name",textboxname1)
         << " Read commands from file: "
         << cgicc::input().set("type","text")
      .set("value",textbox_default_value2)
      .set("name",textboxname2)
         << endl << cgicc::br() << endl
         << cgicc::textarea().set("style",
                                  std::string("width: 500px; ")
                                  + "margin-top: 1em; "
                                  + "height: 10em; ")
      .set("name", textboxname3)
         << textbox_default_value3
         << cgicc::textarea()
         << cgicc::div();
  }
*/
