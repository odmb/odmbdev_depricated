#include "emu/odmbdev/Buttons.h"

#include <time.h>
#include <ctype.h>
#include <sys/stat.h>

/******************************************************************************
 * Some classes are declared in the header file because they are short and
 * sweet. Check there first!
 *
 * Also note, at the end of this file is a template for new Action
 * sub-classes.
 * 
 *****************************************************************************/

namespace emu { 
  namespace odmbdev {

    string FixLength(unsigned int Number, int Length){
      std::stringstream Stream;
      Stream << std::hex << Number;
      string sNumber = Stream.str();
      for(int cha=0; cha<sNumber.size(); cha++) sNumber[cha] = toupper(sNumber[cha]);
      while(sNumber.size() < Length) sNumber = " " + sNumber;
      return sNumber;
    }

    /**************************************************************************
     * ExecuteVMEDSL
     *
     * A domain-specific-lanaguage for issuing vme commands. 
     *************************************************************************/

    ExecuteVMEDSL::ExecuteVMEDSL(Crate * crate)
      : ThreeTextBoxAction(crate, "Execute VME DSL program")
    {
      cout << " Initializing the DAQMBs, which are VMEModules" << endl; 
      for(vector <DAQMB*>::iterator dmb = dmbs.begin();dmb != dmbs.end(); ++dmb) 
	(*dmb)->start();
      nCommand = 0;
    }
    
    void ExecuteVMEDSL::respond(xgi::Input * in, ostringstream & out) {
      ThreeTextBoxAction::respond(in, out);
      
      nCommand++;
      out<<"****************   VME command "<<nCommand<<"   ***************"<<endl;
      int slot = 15; // hard code a default VME slot number

      //// the arguments for vme_controller ////
      char rcv[2];
      unsigned int addr;
      unsigned short int data;
      int irdwr, TypeCommand=0; 
      // Taking negative irdwr and TypeCommand to represent commands not passed to VME (for loops, etc.) - AD
      //// From VMEController.cc:
      // irdwr:   
      // 0 bufread
      // 1 bufwrite 
      // 2 bufread snd  
      // 3 bufwrite snd 
      // 4 flush to VME (disabled)
      // 5 loop back (disabled)
      // 6 delay

      //////////////////////////////////////////////////////  
      // Open a new output logfile today - KF

      time_t rawtime;
      struct tm *timeinfo;
      time(&rawtime);
      timeinfo = localtime(&rawtime);

      char outputfilename[80];
      strftime( outputfilename, 80, "logfiles/%y_%m_%d_ODMB_testing.log", timeinfo );

      char timestamp[80];
      strftime( timestamp, 80, "%T", timeinfo );

      struct stat statbuffer;
      int writeheader = stat( outputfilename, &statbuffer );
	   
      ofstream logfile;
      logfile.open( outputfilename, ios::app );
      if( writeheader!=0 ) logfile << "# R/W\tCommand\tData\tSlot\tTime\t\tComments" << endl;

	  ////////////////////////////////////////////////////// 
	  istringstream countertext(this->textBoxContent1); //read the number of times to repeat commands and possibly a file containing commands  
      istringstream filetext(this->textBoxContent2); // input a file path to the wimp page, read commands from the file
      bool textFileMode(false); // Option to input a text file from the second box on WIMP
      							// works with file in current directory or full path - JB-F
      string filePath, fileContents, file_line;
      if (filetext.str().size()) { // If you specify a file path on the WIMP page, then we feed it into the string "fileContents" - JB-F
      	textFileMode = true;
      	filePath = filetext.str();
      	ifstream fileFromWIMP(filePath.c_str());
      	while ( fileFromWIMP.good() ) {
          getline (fileFromWIMP,file_line);
      	  fileContents+=file_line;
      	  fileContents+="\n";
    	}
    	fileFromWIMP.close();
      }
      istringstream alltext; // where the input commands will go
	  if (textFileMode) {
	   cout << "Reading from file: " << filePath << endl;
	   alltext.str(fileContents); // assigns content of text file to alltext, after you push "Execute VME DSL Program" - JB-F
      }
      else {
      	cout << "Reading from textbox." << endl;
      	alltext.str(this->textBoxContent3); // get the text from the box                                                                                                      
      }
      string line, buffer;
      getline(countertext,line,'\n');
      const unsigned long repeatNumber=strtoul(line.c_str(),NULL,0);
      std::vector<std::string> allLines(0);
      while (getline(alltext,line,'\n')){
	if(1){//line.size()){//Discard empty lines                                                                                                                                   
	  allLines.push_back(line);
	}
      }

      for(unsigned int repNum=0; repNum<repeatNumber; ++repNum){
	unsigned int loopLevel(0);
	std::vector<unsigned int> loopCounter(0), loopMax(0), loopStart(0);
	for(unsigned int lineNum=0; lineNum<allLines.size(); ++lineNum){
	  line=allLines.at(lineNum);
	  while(line[0]==' ' || line[0]=='\t') line.erase(0,1);
	  istringstream iss(line);

	  // 0 :       End of run, stop execution
	  // 1 :       Binary (simulation file style)
	  // 2,r,R :   Reads in hex
	  // 3,w,W :   Writes in hex
	  // 4,rs,RS : Reads in hex
	  // 5,ws,WS : Writes in hex
	  // 6,bl,BL : Begin loop
	  // 7,el,EL : End loop
	  // 
	  // >3 : Produces a sleep for that number of microseconds
	  // 
	  // <0 : Send an error message to the output and abort further execution.
	  if(line.size()>3) iss >> buffer;
	  else buffer = "#";  // This avoids problems when the line is too short
	  if(buffer=="0")  {
	    out<<"Found EOR, exiting."<<endl;
	    return; // EOR instruction
	  } else if(buffer=="1") { // Expect 32 bits for address and 32 bits for data
	    string addr_str, data_str, tmp_str;
	    
	    while(addr_str.size()<32 && iss.good()){ // read in 32 bits for address
	      iss >> tmp_str;
	      addr_str += tmp_str;
	      //out<<"addr_str:"<<addr_str<<endl;
	    }
	    while(data_str.size()<32 && iss.good()){ // read in 32 bits for data
	      iss >> tmp_str;
	      data_str += tmp_str;
	      //out<<"data_str:"<<data_str<<endl;
	    }
	    if(addr_str.size()!=32 || data_str.size()!=32){
	      out<<"ERROR: address("<<addr_str<<") or data("<<data_str
		 <<") is not 32 bits on line: "<<line<<endl;
	      return;
	    }
	    // 26th and 25th "bits" from right tell read (10) or write (01)
	    irdwr = (addr_str.at(addr_str.size()-26)=='1')? 2 : 3; 
	    addr = BinaryString_to_UInt(addr_str);
	    data = BinaryString_to_UInt(data_str);
	    TypeCommand = irdwr;
	  } else if(buffer=="2" || buffer=="r" || buffer=="R") { // Read in hex
	    iss >> hex >> addr >> hex >> data;	
	    irdwr = 2; TypeCommand = 2;
	  } else if(buffer=="3" || buffer=="w" || buffer=="W") { // Write in hex
	    iss >> hex >> addr >> hex >> data;	
	    irdwr = 3; TypeCommand = 3;
	  } else if(buffer=="4" || buffer=="rs" || buffer=="RS") { // Read in hex with slot
	    iss >> hex >> addr >> hex >> data >> dec >> slot;	
	    irdwr = 2; TypeCommand = 4;
	  } else if(buffer=="5" || buffer=="ws" || buffer=="WS") { // Write in hex with slot
	    iss >> hex >> addr >> hex >> data >> dec >> slot;	
	    irdwr = 3; TypeCommand = 5;
	  } else if(buffer=="6" || buffer=="BL" || buffer=="bl") {
	    ++loopLevel;
	    loopCounter.push_back(1);
	    int thisLoopMax(0);
	    iss >> dec >> thisLoopMax;
	    loopMax.push_back(thisLoopMax);
	    loopStart.push_back(lineNum);
	    irdwr = -1; TypeCommand= -1; //Negative; do not send to VME - AD
	  } else if(buffer=="7" || buffer=="EL" || buffer=="el") {
	    if(loopCounter.at(loopLevel-1)<loopMax.at(loopLevel-1)){
	      lineNum=loopStart.at(loopLevel-1);
	      ++loopCounter.at(loopLevel-1);
	    }else{
	      loopCounter.pop_back();
	      loopMax.pop_back();
	      loopStart.pop_back();
	      --loopLevel;
	    }
	    irdwr = -1; TypeCommand = -1; //Negative; do not send to VME - AD
	  } else { // Anything else is treated as a comment.
	    out  << line << endl;
	    logfile << "# " << line << endl;
	    continue; // Next line, please.
	  }

	  //// We are at the end of official commands, add the rest to comments - KF
	  string comments;
	  getline( iss, comments );
	  while(comments[0]==' ' || comments[0]=='\t') comments.erase(0,1);
	  //// Output type of command is always W or R - KF
	  //// Added command type L for loops - AD
	  string commandtype;
	  switch(irdwr){
	  case 2:
	    commandtype="R";
	    break;
	  case 3:
	    commandtype="W";
	    break;
	  case -1:
	    commandtype="L";
	    break;
	  default:
	    commandtype="Error with command type!";
	    break;
	  }

	  //// If we make it here, we have a VME command to run! ////
	  //Only if TypeCommand>0 due to addition of loop! - AD
	  //printf("TypeCommand: %d\n", TypeCommand);
	  //out << TypeCommand << endl;
	  if(TypeCommand>0){
	    // Set the top bits of address to the slot number
	    unsigned int shiftedSlot = slot << 19;
	    addr = (addr & 0x07ffff) | shiftedSlot;
	    
	    printf("Calling:  vme_controller(%d,%06x,&%04x,{%02x,%02x})  ", irdwr, 
		   (addr & 0xffffff), (data & 0xffff), (rcv[0] & 0xff), (rcv[1] & 0xff));
	    crate->vmeController()->vme_controller(irdwr, addr, &data, rcv); // Send the VME command!
	    // VMEController::print_decoded_vme_address(addr, &data);
	    usleep(1);
	    
	    // If it was a read, then show the result
	    if(irdwr==2) printf("  ==> rcv[1,0] = %02x %02x", (rcv[1] & 0xff), (rcv[0] & 0xff));
	    printf("\n");
	    fflush(stdout);
	    
	    
	    // Output to website
	    addr = (addr & 0x07ffff);
	    unsigned int VMEresult = (rcv[1] & 0xff) * 0x100 + (rcv[0] & 0xff);
	    switch (irdwr) {
	    case 2:
	      out << "R  " << FixLength(addr) << "        "  << FixLength(VMEresult)  << "    "<<comments<<endl;
	      logfile << "RS  " << FixLength(addr) << "        "  << FixLength(VMEresult)  << "    "
		      << timestamp << "\t" << comments<<endl;
	      break;
	    case 3:
	      out << "W  " << FixLength(addr) << "  " << FixLength(data) <<  "          "<<comments<<endl;
	      logfile << "WS  " << FixLength(addr) << "  " << FixLength(data) <<  "          "
		      << timestamp << "\t" << comments<<endl;
	      break;
	    }
	  }
	} // while parsing lines
	out<<endl;
      }

      //// close the logfile -KF
      logfile.close();

    } // End ExecuteVMEDSL::respond



    //    /**************************************************************************
    //     * ActionTemplate
    //     *
    //     *************************************************************************/
    //
    //    ActionTemplate::ActionTemplate(Crate * crate)
    //      : Action(crate)
    //    { /* ... nothing to see here ... */ }
    //
    //    void ActionTemplate::display(xgi::Output * out) {
    //
    //    }
    //
    //    void ActionTemplate::respond(xgi::Input * in, ostringstream & out) {
    //
    //    }



    /**************************************************************************
     * Reads back the user codes
     *
     *************************************************************************/

    ReadBackUserCodes::ReadBackUserCodes(Crate * crate)
      : ButtonAction(crate, "Read back usercodes")
    { /* ... nothing to see here ... */ }


    void ReadBackUserCodes::respond(xgi::Input * in, ostringstream & out) {
      cout<<"*********************   Starting ReadBackUserCodes     *********************"<<endl;
      for(vector <DAQMB*>::iterator dmb = dmbs.begin();
	  dmb != dmbs.end();
	  ++dmb) {
	vector <CFEB> cfebs = (*dmb)->cfebs();
	for(CFEBrevItr cfeb = cfebs.rbegin(); cfeb != cfebs.rend(); ++cfeb) {
	  int cfeb_index = (*cfeb).number();

	  out << " ********************* " << endl
	      << " FEB" << cfeb_index << " : "
	      << " Usercode: " << hex << (*dmb)->febfpgauser(*cfeb) << endl
	      << " Virtex 6 Status: " << (*dmb)->virtex6_readreg(7);
	}

	(*dmb)->RedirectOutput(&out);
	(*dmb)->daqmb_adc_dump();
	(*dmb)->RedirectOutput(&cout);
      }
    }

    /**************************************************************************
     * SetComparatorThresholds
     *
     *************************************************************************/

    SetComparatorThresholds::SetComparatorThresholds(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void SetComparatorThresholds::display(xgi::Output * out) {
      AddButtonWithTextBox(out,
			   "Set Comparator Thresholds (in volts):",
			   "ComparatorThresholds",
			   ".030");
    }

    void SetComparatorThresholds::respond(xgi::Input * in, ostringstream & out) {
      int ComparatorThresholds = getFormValueInt("ComparatorThresholds", in);

      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  (*dmb)->set_comp_thresh(ComparatorThresholds);
	}
    }

    /**************************************************************************
     * SetComparatorThresholdsBroadcast
     *
     *************************************************************************/

    SetComparatorThresholdsBroadcast::SetComparatorThresholdsBroadcast(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void SetComparatorThresholdsBroadcast::display(xgi::Output * out) {
      AddButtonWithTextBox(out,
			   "Set Comparator Thresholds-broadcast (in volts):",
			   "ComparatorThresholds",
			   ".030");
    }

    void SetComparatorThresholdsBroadcast::respond(xgi::Input * in, ostringstream & out) {
      int ComparatorThresholds = getFormValueInt("ComparatorThresholds", in);

      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  (*dmb)->dcfeb_set_comp_thresh_bc(ComparatorThresholds);
	}
    }

    /**************************************************************************
     * SetUpComparatorPulse
     *
     *************************************************************************/

    SetUpComparatorPulse::SetUpComparatorPulse(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void SetUpComparatorPulse::display(xgi::Output * out) {
      AddButtonWithTextBox(out,
			   "Set up internal capacitor pulse on halfstrip:",
			   "halfstrip",
			   "16");
    }

    void SetUpComparatorPulse::respond(xgi::Input * in, ostringstream & out) {
      int halfstrip = getFormValueInt("halfstrip", in);

      ccb->hardReset();
      sleep(5);

      tmb->SetClctPatternTrigEnable(1);
      tmb->WriteRegister(emu::pc::seq_trig_en_adr);

      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  int hp[6] = {halfstrip+1, halfstrip, halfstrip+1, halfstrip, halfstrip+1, halfstrip};
	  (*dmb)->trigsetx(hp,0x7f);
	  (*dmb)->buck_shift();
	}

      ccb->syncReset();//check
      sleep(1);
      ccb->bx0();   //check
    }

    /**************************************************************************
     * SetUpPrecisionCapacitors
     *
     *************************************************************************/

    SetUpPrecisionCapacitors::SetUpPrecisionCapacitors(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void SetUpPrecisionCapacitors::display(xgi::Output * out) {
      AddButtonWithTextBox(out,
			   "Set up precision capacitor pulse on strip:",
			   "StripToPulse",
			   "8");
    }

    void SetUpPrecisionCapacitors::respond(xgi::Input * in, ostringstream & out) {
      int strip_to_pulse = getFormValueInt("StripToPulse", in);

      ccb->hardReset();
      sleep(5);

      tmb->SetClctPatternTrigEnable(1);
      tmb->WriteRegister(emu::pc::seq_trig_en_adr);

      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  (*dmb)->set_ext_chanx(strip_to_pulse);//check
	  (*dmb)->buck_shift();//check
	}

      ccb->syncReset();//check
      sleep(1);
      ccb->bx0();
    }

    /**************************************************************************
     * PulseInternalCapacitors
     *
     *************************************************************************/

    PulseInternalCapacitors::PulseInternalCapacitors(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void PulseInternalCapacitors::display(xgi::Output * out) {
      AddButton(out, "Pulse internal capacitors via DMB");
    }

    void PulseInternalCapacitors::respond(xgi::Input * in, ostringstream & out) {
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  (*dmb)->inject(1,0);
	}
    }

    /**************************************************************************
     * PulseInternalCapacitorsCCB
     *
     *************************************************************************/

    PulseInternalCapacitorsCCB::PulseInternalCapacitorsCCB(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void PulseInternalCapacitorsCCB::display(xgi::Output * out) {
      AddButton(out, "Pulse internal capacitors via CCB");
    }

    void PulseInternalCapacitorsCCB::respond(xgi::Input * in, ostringstream & out) {
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  ccb->inject(1,0);
	}
    }

    /**************************************************************************
     * PulsePrecisionCapacitors
     *
     *************************************************************************/

    PulsePrecisionCapacitors::PulsePrecisionCapacitors(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void PulsePrecisionCapacitors::display(xgi::Output * out) {
      AddButton(out, "Pulse precision capacitors via DMB");
    }

    void PulsePrecisionCapacitors::respond(xgi::Input * in, ostringstream & out) {
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  (*dmb)->pulse(1,0);
	}
    }

    /**************************************************************************
     * PulsePrecisionCapacitorsCCB
     *
     *************************************************************************/

    PulsePrecisionCapacitorsCCB::PulsePrecisionCapacitorsCCB(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void PulsePrecisionCapacitorsCCB::display(xgi::Output * out) {
      AddButton(out, "Pulse precision capacitors via CCB");
    }

    void PulsePrecisionCapacitorsCCB::respond(xgi::Input * in, ostringstream & out) {
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  ccb->pulse(1,0);
	}
    }

    /**************************************************************************
     * SetDMBDACs
     *
     *************************************************************************/

    SetDMBDACs::SetDMBDACs(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void SetDMBDACs::display(xgi::Output * out) {
      AddButtonWithTextBox(out,
			   "Set DMB DACs 0 and 1 to (in volts):",
			   "DAC",
			   "1.0");
    }

    void SetDMBDACs::respond(xgi::Input * in, ostringstream & out) {
      float DAC = getFormValueFloat("DAC", in);
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  (*dmb)->set_dac(DAC,DAC);
	}
    }

    /**************************************************************************
     * ShiftBuckeyesNormRun
     *
     *************************************************************************/

    ShiftBuckeyesNormRun::ShiftBuckeyesNormRun(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void ShiftBuckeyesNormRun::display(xgi::Output * out) {
      AddButton(out, "Shift Buckeyes into normal mode");
    }

    void ShiftBuckeyesNormRun::respond(xgi::Input * in, ostringstream & out) {
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  (*dmb)->shift_all(NORM_RUN);
	}
    }

    /**************************************************************************
     * SetPipelineDepthAllDCFEBs
     *
     *************************************************************************/

    SetPipelineDepthAllDCFEBs::SetPipelineDepthAllDCFEBs(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void SetPipelineDepthAllDCFEBs::display(xgi::Output * out) {
      AddButtonWithTextBox(out,
			   "Set pipeline depth on all DCFEBs:",
			   "depth",
			   "61");
    }

    void SetPipelineDepthAllDCFEBs::respond(xgi::Input * in, ostringstream & out) {
      int depth = getFormValueInt("depth", in);
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  vector <CFEB> cfebs = (*dmb)->cfebs();
	  for(CFEBrevItr cfeb = cfebs.rbegin(); cfeb != cfebs.rend(); ++cfeb)
	    {
	      (*dmb)->dcfeb_set_PipelineDepth(*cfeb, depth);
	    }
	}
    }

    /**************************************************************************
     * SetFineDelayForADCFEB
     *
     *************************************************************************/

    SetFineDelayForADCFEB::SetFineDelayForADCFEB(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void SetFineDelayForADCFEB::display(xgi::Output * out) {
      AddButtonWithTwoTextBoxes(out,
				"Set Fine Delay on FEB(0-4) to (0-15):",
				"DcfebNumber",
				"1",
				"FineDelay",
				"0");
    }

    void SetFineDelayForADCFEB::respond(xgi::Input * in, ostringstream & out) {
      int delay = getFormValueInt("FineDelay", in);
      int cfeb_number = getFormValueInt("DcfebNumber", in);
      for(vector <DAQMB*>::iterator dmb = dmbs.begin(); dmb != dmbs.end(); ++dmb)
	{
	  vector <CFEB> cfebs = (*dmb)->cfebs();
	  (*dmb)->dcfeb_fine_delay(cfebs.at(cfeb_number), delay);
	  usleep(100);
	  (*dmb)->Pipeline_Restart(cfebs[cfeb_number]);
	}
    }

    /**************************************************************************
     * TMBHardResetTest
     *
     *************************************************************************/

    TMBHardResetTest::TMBHardResetTest(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void TMBHardResetTest::display(xgi::Output * out) {
      AddButtonWithTextBox(out,
			   "TMB Hard Reset Test, number of resets:",
			   "NumberOfHardResets",
			   "100");
    }

    void TMBHardResetTest::respond(xgi::Input * in, ostringstream & out) {
      int NumberOfHardResets = getFormValueInt("NumberOfHardResets", in);
      out << "=== TMB Hard Reset Test ===\n";

      int expected_day = tmb->GetExpectedTmbFirmwareDay();
      int expected_month = tmb->GetExpectedTmbFirmwareMonth();
      int expected_year = tmb->GetExpectedTmbFirmwareYear();
      int expected_type = tmb->GetExpectedTmbFirmwareType();
      int expected_version = tmb->GetExpectedTmbFirmwareVersion();

      int i; // we'll want to get the value of this after the loop is complete
      // in order to print how many succesful hard resets we ran
      bool firmware_lost = false;

      // the CCB writes to stdout every time it issues a hard rest, but we
      // don't care we turn this back on after the loop
      ostringstream waste;
      ccb->RedirectOutput(&waste);

      for (i = 0;
	   i < NumberOfHardResets && !firmware_lost;
	   ++i)
	{
	  if (i % 500 == 0) {
	    out << "Hard Reset Number " << i << endl;
	  }

	  ccb->hardReset();

	  const int maximum_firmware_readback_attempts = 2;
	  int firmware_readback_attempts = 0;
	  do {
	    tmb->FirmwareDate(); // reads the month and day off of the tmb
	    int actual_day = tmb->GetReadTmbFirmwareDay();
	    int actual_month = tmb->GetReadTmbFirmwareMonth();
	    tmb->FirmwareYear(); // reads the year off of the tmb
	    int actual_year = tmb->GetReadTmbFirmwareYear();
	    tmb->FirmwareVersion(); // reads the version off of the tmb
	    int actual_type = tmb->GetReadTmbFirmwareType();
	    int actual_version = tmb->GetReadTmbFirmwareVersion();

	    if ((actual_day != expected_day) ||
		(actual_month != expected_month) ||
		(actual_year != expected_year) ||
		(actual_type != expected_type) ||
		(actual_version != expected_version)) {
	      firmware_lost = true;
	      sleep(1); // sometimes the readback fails, so wait one second and
	      // try again
	    }

	    // if we haven't gone over our maximum number of readback attempts and
	    // the firmware was "lost" (i.e. the readback didn't match the expected
	    // values), then try again.
	    // NB: ++x increments x before evaluating the boolean expression
	  } while (++firmware_readback_attempts < maximum_firmware_readback_attempts &&
		   firmware_lost);
	}

      ccb->RedirectOutput(&cout);

      if(firmware_lost) {
	out << "The frimware was lost after " << i << " hard resets." << endl;
      } else {
	out << "The firmware was *never* lost after " << i << " hard resets." << endl;
      }
    }

    /**************************************************************************
     * DDUReadKillFiber
     *
     *************************************************************************/

    DDUReadKillFiber::DDUReadKillFiber(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void DDUReadKillFiber::display(xgi::Output * out) {
      AddButton(out, "Read DDU Kill Fiber");
    }

    void DDUReadKillFiber::respond(xgi::Input * in, ostringstream & out) {
      out << "=== DDU Read Kill Fiber ===" << endl;

      for(vector <DDU*>::iterator ddu = ddus.begin();
	  ddu != ddus.end();
	  ++ddu) {
	out << "DDU with ctrl fpga user code: " << (*ddu)->CtrlFpgaUserCode()
	    << hex << setfill('0') // set up for next two hex values
	    << " and vme prom user code: "
	    << setw(8) << (*ddu)->VmePromUserCode()
	    << " has Kill Fiber is set to: "
	    << setw(4) << (*ddu)->readFlashKillFiber() << endl;
      }
    }

    /**************************************************************************
     * DDUWriteKillFiber
     *
     *************************************************************************/

    DDUWriteKillFiber::DDUWriteKillFiber(Crate * crate)
      : Action(crate)
    { /* ... nothing to see here ... */ }

    void DDUWriteKillFiber::display(xgi::Output * out) { 
      AddButtonWithTextBox(out,
			   "Write DDU Kill Fiber (in hex, 15 bits)",
			   "KillFiber",
			   "7fff");
    }

    void DDUWriteKillFiber::respond(xgi::Input * in, ostringstream & out) {
      int KillFiber = getFormValueIntHex("KillFiber", in);

      out << "=== DDU Write Kill Fiber ===" << endl;

      for(vector <DDU*>::iterator ddu = ddus.begin();
	  ddu != ddus.end();
	  ++ddu) {
	(*ddu)->writeFlashKillFiber(KillFiber);
      }
    }

  }
}
