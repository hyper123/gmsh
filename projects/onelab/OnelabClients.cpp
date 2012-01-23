#include "OnelabMessage.h"
#include "OnelabClients.h"

class onelabServer : public GmshServer{
 private:
  onelab::localNetworkClient *_client;
 public:
  onelabServer(onelab::localNetworkClient *client) : GmshServer(), _client(client) {}
  ~onelabServer() {}
  int SystemCall(const char *str)
  { 
    printf("ONELAB System call(%s)\n", str);
    return system(str); 
  }
  int NonBlockingWait(int socket, double waitint, double timeout)
  { 
    double start = GetTimeInSeconds();
    while(1){
      if(timeout > 0 && GetTimeInSeconds() - start > timeout)
        return 2; // timeout
      if(_client->getPid() < 0)
        return 1; // process has been killed

      // check if there is data (call select with a zero timeout to
      // return immediately, i.e., do polling)
      int ret = Select(0, 0, socket);
      if(ret == 0){ 
        // nothing available: wait at most waitint seconds
      }
      else if(ret > 0){ 
        return 0; // data is there!
      }
      else{ 
        // an error happened
        _client->setPid(-1);
	_client->setGmshServer(0);
        return 1;
      }
    }
  }
};

bool onelab::localNetworkClient::run()
{
 new_connection:
  _pid = 0;
  _gmshServer = 0;

  onelabServer *server = new onelabServer(this);
 
  //std::string socketName = ":";
  std::string socketName = getUserHomedir() + ".gmshsock";
  std::string sockname;
  std::ostringstream tmp;
  if(!strstr(socketName.c_str(), ":")){
    // Unix socket
    tmp << socketName << getId();
    //sockname = FixWindowsPath(tmp.str());
  }
  else{
    // TCP/IP socket
    if(socketName.size() && socketName[0] == ':')
      tmp << GetHostName(); // prepend hostname if only the port number is given
    //tmp << socketName << getId();
    tmp << socketName ;
  }
  sockname = tmp.str();

  std::string command = _commandLine;
  if(command.size()){
    std::vector<onelab::string> ps;
    get(ps, getName() + "/Action");
    std::string action = (ps.empty() ? "" : ps[0].getValue());
    get(ps, getName() + "/1ModelName");
    std::string modelName = (ps.empty() ? "" : ps[0].getValue());
    get(ps, getName() + "/9LineOptions");
    std::string options = (ps.empty() ? "" : ps[0].getValue());
    get(ps, getName() + "/9InitializeCommand");
    std::string initializeCommand = (ps.empty() ? "" : ps[0].getValue());
    get(ps, getName() + "/9CheckCommand");
    std::string checkCommand = (ps.empty() ? "" : ps[0].getValue());
    get(ps, getName() + "/9ComputeCommand");
    std::string computeCommand = (ps.empty() ? "" : ps[0].getValue());

    if(action == "initialize")
      command += " " + initializeCommand;
    else if(action == "check")
      command += " " + modelName + " " + options + " " + checkCommand;
    else if(action == "compute")
      command += " " + modelName + " " + options + " " + computeCommand;
    else
      Msg::Fatal("localNetworkClient::run: Unknown: Unknown Action <%s>", action.c_str());

    // append "-onelab" command line argument
    command += " " + _socketSwitch + " \"" + getName() + "\"";
  }
  else{
    Msg::Info("Listening on socket '%s'", sockname.c_str());
  }

  // std::cout << "FHF sockname=" << sockname.c_str() << std::endl;
  // std::cout << "FHF command=" << command.c_str() << std::endl;

  int sock;
  try{
    sock = server->Start(command.c_str(), sockname.c_str(), 10);
  }
  catch(const char *err){
    Msg::Error("%s (on socket '%s')", err, sockname.c_str());
    sock = -1;
  }

  if(sock < 0){
    server->Shutdown();
    delete server;
    return false;
  }

  Msg::StatusBar(2, true, "ONELAB: Now running client '%s'...", _name.c_str());
  while(1) {
    if(_pid < 0) break;
    
    int stop = server->NonBlockingWait(sock, 0.1, 0.);
    if(stop || _pid < 0) {
      Msg::Info("Stop=%d _pid=%d",stop, _pid);
      break;
    }
    int type, length, swap;
    if(!server->ReceiveHeader(&type, &length, &swap)){
      Msg::Error("Did not receive message header: stopping server");
      break;
    }
    // else
    //   std::cout << "FHF: Received header=" << type << std::endl;

    std::string message(length, ' ');
    if(!server->ReceiveMessage(length, &message[0])){
      Msg::Error("Did not receive message body: stopping server");
      break;
    }
    // else
    //   std::cout << "FHF: Received message=" << message << std::endl;

    switch (type) {
    case GmshSocket::GMSH_START:
      _pid = atoi(message.c_str());
      _gmshServer = server;
      break;
    case GmshSocket::GMSH_STOP:
      _pid = -1;
      _gmshServer = 0;
      break;
    case GmshSocket::GMSH_PARAMETER:
      {
        std::string version, type, name;
        onelab::parameter::getInfoFromChar(message, version, type, name);
        if(type == "number"){
          onelab::number p;
          p.fromChar(message);
          set(p);
        }
        else if(type == "string"){
          onelab::string p;
          p.fromChar(message);
          set(p);
        }
        else
          Msg::Fatal("FIXME query not done for this parameter type: <%s>", message.c_str());
      }
      break;
    case GmshSocket::GMSH_PARAMETER_QUERY:
      {
        std::string version, type, name;
        onelab::parameter::getInfoFromChar(message, version, type, name);
        if(type == "number"){
          std::vector<onelab::number> par;
          get(par, name);
          if(par.size() == 1){
            std::string reply = par[0].toChar();
            server->SendMessage(GmshSocket::GMSH_PARAMETER, reply.size(), &reply[0]);
          }
          else{
            std::string reply = "Parameter (number) " + name + " not found";
            server->SendMessage(GmshSocket::GMSH_INFO, reply.size(), &reply[0]);
          }
        }
        else if(type == "string"){
          std::vector<onelab::string> par;
          get(par, name);
          if(par.size() == 1){
            std::string reply = par[0].toChar();
            server->SendMessage(GmshSocket::GMSH_PARAMETER, reply.size(), &reply[0]);
          }
          else{
            std::string reply = "Parameter (string) " + name + " not found";
            server->SendMessage(GmshSocket::GMSH_INFO, reply.size(), &reply[0]);
          }
        }
        else
          Msg::Fatal("FIXME query not done for this parameter type: <%s>", message.c_str());
      }
      break;
    case GmshSocket::GMSH_PARAM_QUERY_ALL:
      {
        std::string version, type, name, reply;
        onelab::parameter::getInfoFromChar(message, version, type, name);
	if(type == "number"){
	  std::vector<onelab::number> numbers;
	  get(numbers, "");
	  for(std::vector<onelab::number>::iterator it = numbers.begin(); it != numbers.end(); it++){
	    reply = (*it).toChar();
	    server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_ALL, reply.size(), &reply[0]);
	  }
	  server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_END, 0, NULL);
	}
	else if(type == "string"){
	  std::vector<onelab::string> strings;
	  get(strings, "");
	  for(std::vector<onelab::string>::iterator it = strings.begin(); it != strings.end(); it++){
	    reply = (*it).toChar();
	    server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_ALL, reply.size(), &reply[0]);
	  }
	  server->SendMessage(GmshSocket::GMSH_PARAM_QUERY_END, 0, NULL);
	}
        else
          Msg::Fatal("FIXME query not done for this parameter type: <%s>", message.c_str());
      }
      break;
    case GmshSocket::GMSH_PROGRESS:
      Msg::StatusBar(2, false, "%s %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_INFO:
      Msg::Direct("%-8.8s: %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_WARNING:
      Msg::Direct(2, "%-8.8s: %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_ERROR:
      //Msg::Direct(1, "%-8.8s: %s", _name.c_str(), message.c_str());
      Msg::Fatal("%-8.8s: %s", _name.c_str(), message.c_str());
      break;
    case GmshSocket::GMSH_MERGE_FILE:
      SystemCall("gmsh "+ message+" &");
      break;
    default:
      Msg::Warning("Received unknown message type (%d)", type);
      break;
    }
  }

  server->Shutdown();
  delete server;
  Msg::StatusBar(2, true, "ONELAB: Done running '%s'", _name.c_str());
  return true;
}

bool onelab::localNetworkClient::kill()
{
  if(_pid > 0) {
    if(KillProcess(_pid)){
      Msg::Info("Killed '%s' (pid %d)", _name.c_str(), _pid);
      _pid = -1;
      return true; 
    }
  }
  _pid = -1;
  return false;
}

void MetaModel::registerClient(onelab::client *pName)
{ 
  _clients.push_back(pName);
  Msg::Info("Register client <%s>", pName->getName().c_str());
}

void MetaModel::initialize()
{
  Msg::Info("Metamodel::initialize <%s>",getName().c_str());
  Msg::SetOnelabString(clientName + "/9CheckCommand","-a");
  Msg::SetOnelabNumber(clientName + "/Initialized",1);
  Msg::SetOnelabNumber(clientName + "/UseCommandLine",1);
}

void MetaModel::initializeClients()
{
  Msg::Info("Metamodel::initializeClients <%s>",getName().c_str());
  for(unsigned int i = 0; i < _clients.size(); i++){
    Msg::SetOnelabString(_clients[i]->getName()+"/Action","initialize");
    std::cout << "MetaModel::initialize " << _clients[i]->getName() << std::endl;
    _clients[i]->run();
  }
}
// void MetaModel::automaticCheck()
// {
//   initializeClients();
//   for(unsigned int i = 0; i < _clients.size(); i++){
//    Msg::SetOnelabString(_clients[i]->getName()+"/Action","check");
//    std::cout << "MetaModel::check " << _clients[i]->getName() << std::endl;
//    _clients[i]->run();
//   }
// }

// MetaModel::analyze() et MetaModel::compute()
// sont définies par l'utilisateur dans un fichier à part

int PromptUser::getVerbosity(){
  std::vector<onelab::number> numbers;
  get(numbers,"VERBOSITY");
  if (numbers.size())
    return numbers[0].getValue();
  else
    return 0;
}
void PromptUser::setVerbosity(const int ival){
  onelab::number number;
  number.setName("VERBOSITY");
  number.setValue(ival);
  set(number);
}

void PromptUser::setNumber(const std::string paramName, const double val, const std::string &help){
  onelab::number number;
  number.setName(paramName);
  number.setValue(val);
  number.setHelp(help);
  set(number);
}
double PromptUser::getNumber(const std::string paramName){
  std::vector<onelab::number> numbers;
  get(numbers,paramName);
  if (numbers.size())
    return numbers[0].getValue();
  else
    Msg::Fatal("Unknown parameter %s",paramName.c_str());
}
bool PromptUser::existNumber(const std::string paramName){
  std::vector<onelab::number> numbers;
  get(numbers,paramName);
  return numbers.size();
}
void PromptUser::setString(const std::string paramName, const std::string &val, const std::string &help){
  onelab::string string;
  string.setName(paramName);
  string.setValue(val);
  string.setHelp(help);
  set(string);
}
std::string PromptUser::getString(const std::string paramName){
  std::vector<onelab::string> strings;
  get(strings,paramName);
  if (strings.size())
    return strings[0].getValue();
  else
    Msg::Fatal("Unknown parameter %s",paramName.c_str());
}
bool PromptUser::existString(const std::string paramName){
  std::vector<onelab::string> strings;
  get(strings,paramName);
  return strings.size();
}
// std::vector<std::string> PromptUser::getChoices(const std::string paramName){
//   std::vector<onelab::string> strings;
//   get(strings,paramName);
//   if (strings.size())
//     return strings[0].getChoices();
//   else
//     Msg::Fatal("Unknown parameter %s",paramName.c_str());
// }

std::string PromptUser::stateToChar(){
  std::vector<onelab::number> numbers;
  std::ostringstream sstream;
  get(numbers);
  for(std::vector<onelab::number>::iterator it = numbers.begin();
      it != numbers.end(); it++)
    sstream << (*it).getValue() << '\t';
  return sstream.str();
}

std::string PromptUser::showParamSpace(){
  std::string db = "ONELAB parameter space: size=" + itoa(onelab::server::instance()->getNumParameters()) + "\n";
  db.append(onelab::server::instance()->toChar());
  for(unsigned int i = 0; i < db.size(); i++)
    if(db[i] == onelab::parameter::charSep()) db[i] = '|';
  return db.c_str();
}

bool PromptUser::menu(std::string commandLine, std::string fileName, int modelNumber) { 
  int choice, counter1=0, counter2=0;
  std::string answ;
  std::string name;
  onelab::number x;
  onelab::string y;
  std::vector<onelab::number> numbers;
  std::vector<onelab::string> strings;

  EncapsulatedClient *loadedSolver = new  EncapsulatedClient("MetaModel",commandLine,"");
  setString("Arguments/FileName",fileName);

  do {
    std::cout << "\nONELAB: menu" << std::endl ;
    std::cout << " 1- View parameter space\n 2- Set a value\n 3- List modified files\n 4- Initialize\n 5- Analyze\n 6- Compute\n 7- Quit metamodel" << std::endl;

    choice=0;
    std::string mystr;
    while( (choice<1 || choice>6) && ++counter1<10 ) {
      std::cout << "\nONELAB: your choice? "; 
      std::getline (std::cin, mystr);
      std::stringstream myStream(mystr);
      if (myStream >> choice) break;
      std::cout << "Invalid choice" << std::endl;
    }
    std::cout << "Your choice is <" << choice << ">" << std::endl;

    if (choice==1){
      std::cout << "\nONELAB: Present state of the parameter space\n" << std::endl;
      std::string db = onelab::server::instance()->toChar();
      for(unsigned int i = 0; i < db.size(); i++)
	if(db[i] == onelab::parameter::charSep()) db[i] = '|';
      std::cout << db.c_str();
      choice=0;
    }
    else if (choice==2){
      std::cout << "ONELAB: Variable name? "; std::cin >> name;
      get(numbers,name);
      if (numbers.size()) {
	float fval;
	std::cout << "ONELAB: Value? "; std::cin >> fval;
	numbers[0].setValue(fval);
	bool allowed = set(numbers[0]);
      }
      else{
	get(strings,name);
	if (strings.size()) {
	  std::string sval;
	  std::cout << "ONELAB: Value? "; std::cin >> sval;
	  strings[0].setValue(sval);
	  bool allowed = set(strings[0]);
	  std::cout << "ONELAB: Ok you are allowed to modify this!" << std::endl;
	}
	else
	  std::cout << "ONELAB: The variable " << name << " is not defined" << std::endl;
      }
      choice=0;
    }
    else if (choice==3){
      std::ifstream infile("onelab.modified");
      std::string buff;
      if (infile.is_open()){
	while ( infile.good() ) {
	  getline (infile,buff);
	  std::cout << buff << std::endl;
	}
      }
      choice=0;
    }
    else if (choice==4){
      loadedSolver->initialize();
      choice=0;
    }
    else if (choice==5){
      loadedSolver->analyze();
      choice=0;
    }
    else if (choice==6){
      loadedSolver->compute();
      choice=0;
    }
    else if (choice==7)
      exit(1);
    else
      choice=0;
  } while(!choice && ++counter2<20);
}

std::string sanitize(const std::string &in)
{
  std::string out, forbidden(" ();");
  for(unsigned int i = 0; i < in.size(); i++)
    if ( forbidden.find(in[i]) == std::string::npos)
      out.push_back(in[i]);
  return out;
}
int enclosed(const std::string &in, std::vector<std::string> &arguments){
  int pos, cursor;
  arguments.resize(0);
  cursor=0;
  if ( (pos=in.find("(",cursor)) == std::string::npos )
     Msg::Fatal("Onelab syntax error: <%s>",in.c_str());
  cursor = pos+1;
  while( (pos=in.find(",",cursor)) != std::string::npos ){
    arguments.push_back(in.substr(cursor,pos-cursor));
    cursor = pos+1;
  }
  if ( (pos=in.find(")",cursor)) == std::string::npos )
     Msg::Fatal("Onelab syntax error: <%s>",in.c_str());
  else
    arguments.push_back(in.substr(cursor,pos-cursor));
  return arguments.size();
}
int extract(const std::string &in, std::string &paramName, std::string &action, std::vector<std::string> &arguments){
  int pos, cursor,NumArg=0;
  cursor=0;
  if ( (pos=in.find(".",cursor)) == std::string::npos )
     Msg::Fatal("Onelab syntax error: <%s>",in.c_str());
  else
    paramName.assign(sanitize(in.substr(cursor,pos-cursor)));
  cursor = pos+1;
  if ( (pos=in.find("(",cursor)) == std::string::npos )
     Msg::Fatal("Onelab syntax error: <%s>",in.c_str());
  else
    action.assign(sanitize(in.substr(cursor,pos-cursor)));
  cursor = pos;
  if ( (pos=in.find(")",cursor)) == std::string::npos )
     Msg::Fatal("Onelab syntax error: %s",in.c_str());
  else
    NumArg = enclosed(in.substr(cursor,pos+1-cursor),arguments);
  return NumArg;
}

// reserved keywords for onelab 
std::string onelabLabel("onelab"), param(onelabLabel+".parameter");
std::string number(onelabLabel+".number"), string(onelabLabel+".string"), include(onelabLabel+".include"); 
std::string iftrue(onelabLabel+".iftrue"), olelse(onelabLabel+".else"), olendif(onelabLabel+".endif"); 
std::string ifequal(onelabLabel+".ifequal");
std::string getValue(onelabLabel+".getValue");

bool InterfacedClient::analyze_oneline(std::string line, std::ifstream &infile) { 
  std::vector<onelab::number> numbers;
  std::vector<onelab::string> strings;
  std::vector<std::string> arguments;
  int pos0,pos,cursor;
  char sep=';';
  std::string buff;
  std::set<std::string>::iterator it;

  if ( (pos=line.find(param)) != std::string::npos) {// onelab.param
    cursor = pos+param.length();
    while ( (pos=line.find(sep,cursor)) != std::string::npos){
      std::string name, action;
      extract(line.substr(cursor,pos-cursor),name,action,arguments);

      if(!action.compare("number")) { // paramName.number(val,path,help,...)
	if(arguments.empty())
	  Msg::Fatal("ONELAB: No value given for param <%s>",name.c_str());
	double val=atof(arguments[0].c_str());
	if(arguments.size()>1){
	  name.assign(arguments[1] + name);
	}
	_parameters.insert(name);
	get(numbers,name);
	if(numbers.empty()){ // if param does not exist yet, value is set
	  numbers.resize(1);
	  numbers[0].setName(name);
	  numbers[0].setValue(val);
	}
	if(arguments.size()>2)
	  numbers[0].setHelp(arguments[2]);
	set(numbers[0]);
	cursor=pos+1;
      }
      else if(!action.compare("string")) { // paramName.string(val,path,help)
	if(arguments.empty())
	  Msg::Fatal("ONELAB: No value given for param <%s>",name.c_str());
	std::string value=arguments[0];
	if(arguments.size()>1)
	  name.assign(arguments[1] + name);
	_parameters.insert(name);
	get(strings,name); 
	if(strings.empty()){ //  if param does not exist yet, value is set
	  strings.resize(1);
	  strings[0].setName(name);
	  strings[0].setValue(value);
	}
	if(arguments.size()>2)
	  strings[0].setHelp(arguments[2]);
	set(strings[0]);
      }
      else if(!action.compare("AddChoices")){
	if(arguments.size()){
	  if((it = _parameters.find(name)) == _parameters.end()) // find complete name inclusive path
	    Msg::Fatal("ONELAB AddChoices: unknown variable: %s",name.c_str());
	  name.assign(*it);
	  get(numbers,name);
	  if(numbers.size()){
	    std::vector<double> choices=numbers[0].getChoices();
	    for(unsigned int i = 0; i < arguments.size(); i++){
	      double val=atof(arguments[i].c_str());
	      if(std::find(choices.begin(),choices.end(),val)==choices.end())
		choices.push_back(val);
	    }
	    numbers[0].setChoices(choices);
	    set(numbers[0]);
	  }
	  else{
	    get(strings,name); 
	    if(strings.size()){
	      std::vector<std::string> choices=strings[0].getChoices();
	      for(unsigned int i = 0; i < arguments.size(); i++)
		if(std::find(choices.begin(),choices.end(),arguments[i])==choices.end())
		   choices.push_back(arguments[i]);
	      strings[0].setChoices(choices);
	      set(strings[0]);
	    }
	    else{
	      Msg::Fatal("ONELAB: the parameter <%s> does not exist",name.c_str());
	    }
	  }
	}
      }
      else if(!action.compare("SetValue")){
	if(arguments.empty())
	  Msg::Fatal("ONELAB: missing argument SetValue <%s>",name.c_str());
	if((it = _parameters.find(name)) == _parameters.end()) // find complete name inclusive path
	  Msg::Fatal("ONELAB SetValue: unknown variable: %s",name.c_str());
	name.assign(*it);
	get(numbers,name); 
	if(numbers.size()){ //  if param does not exist yet, value is set
	  numbers[0].setValue(atof(arguments[0].c_str()));
	  set(numbers[0]);
	}
	else{
	  get(strings,name); 
	  if(strings.size()){
	    strings[0].setValue(arguments[0]);
	    set(strings[0]);
	  }
	  else{
	    Msg::Fatal("ONELAB: the parameter <%s> does not exist",name.c_str());
	  }
	}
      }
      else
	Msg::Fatal("ONELAB: unknown action <%s>",action.c_str());
      cursor=pos+1;
    }
  }
  else if ( (pos=line.find(iftrue)) != std::string::npos) {// onelab.iftrue
    cursor = pos+iftrue.length();
    pos=line.find_first_of(')',cursor)+1;
    if(enclosed(line.substr(cursor,pos-cursor),arguments)<1)
      Msg::Fatal("ONELAB misformed <%s> statement: (%s)",iftrue.c_str(),line.c_str());
    bool condition = false;
    if((it = _parameters.find(arguments[0])) == _parameters.end()) // find complete name inclusive path
      Msg::Fatal("ONELAB %s: unknown variable: %s",iftrue.c_str(),arguments[0].c_str());
    get(numbers,*it);
    if (numbers.size())
      condition = (bool) numbers[0].getValue();
    if (!analyze_ifstatement(infile,condition))
      Msg::Fatal("ONELAB misformed <%s> statement: %s",iftrue.c_str(),arguments[0].c_str());     
  }
  else if ( (pos=line.find(ifequal)) != std::string::npos) {// onelab.ifequal
    cursor = pos+ifequal.length();
    pos=line.find_first_of(')',cursor)+1;
    if (enclosed(line.substr(cursor,pos-cursor),arguments) <2)
      Msg::Fatal("ONELAB misformed <%s> statement: (%s)",ifequal.c_str(),line.c_str());
    bool condition= false;
    if((it = _parameters.find(arguments[0])) == _parameters.end()) // find complete name inclusive path
      Msg::Fatal("ONELAB %s: unknown variable: %s",ifequal.c_str(),arguments[0].c_str());
    get(strings,*it);
    if (strings.size())
      condition= !strings[0].getValue().compare(arguments[1]);
    if (!analyze_ifstatement(infile,condition))
      Msg::Fatal("ONELAB misformed <%s> statement: (%s,%s)",ifequal.c_str(),arguments[0].c_str(),arguments[1].c_str());
  }
  else if ( (pos=line.find(include)) != std::string::npos) {// onelab.include
    cursor = pos+include.length();
    pos=line.find_first_of(')',cursor)+1;
    if(enclosed(line.substr(cursor,pos-cursor),arguments)<1)
      Msg::Fatal("ONELAB misformed <%s> statement: (%s)",include.c_str(),line.c_str());
    analyze_onefile(arguments[0]);
  }
}

bool InterfacedClient::analyze_onefile(std::string ifilename) { 
  std::vector<onelab::number> numbers;
  std::vector<onelab::string> strings;
  int pos0,pos,cursor;
  char sep=';';
  std::string line,buff;
  std::ifstream infile(ifilename.c_str());

  analyze_oneline("onelab.parameter OutputFiles.string(dummy,"+getName()+"/9,Output files);",infile);
  if (infile.is_open()){
    while ( infile.good() ) {
      getline (infile,line);
      analyze_oneline(line,infile);
    }
    infile.close();
  }
  else
    Msg::Fatal("The file %s cannot be opened",ifilename.c_str());
} 

bool InterfacedClient::analyze_ifstatement(std::ifstream &infile, bool condition) { 
  int pos;
  std::string line;

  bool trueclause=true, statementend=false;
  while ( infile.good() && !statementend) {
    getline (infile,line);
    if ( (pos=line.find(olelse)) != std::string::npos) 
      trueclause=false;
    else if ( (pos=line.find(olendif)) != std::string::npos) 
      statementend=true;
    else if ( !(trueclause ^ condition) ) // xor bitwise operator
      analyze_oneline(line,infile);
  }
  return statementend;
} 

bool InterfacedClient::convert_ifstatement(std::ifstream &infile, std::ofstream &outfile, bool condition) { 
  int pos;
  std::string line;

  bool trueclause=true, statementend=false;
  while ( infile.good() && !statementend) {
    getline (infile,line);
    if ( (pos=line.find(olelse)) != std::string::npos) 
      trueclause=false;
    else if ( (pos=line.find(olendif)) != std::string::npos) 
      statementend=true;
    else if ( !(trueclause ^ condition) ) // xor bitwise operator
      convert_oneline(line,infile,outfile);
  }
  return statementend;
} 

bool InterfacedClient::convert_oneline(std::string line, std::ifstream &infile, std::ofstream &outfile) { 
  std::vector<onelab::number> numbers;
  std::vector<onelab::string> strings;
  std::vector<std::string> arguments;
  int pos0,pos,cursor;
  char sep=';';
  std::string buff;
  std::set<std::string>::iterator it;

  if ( (pos=line.find(onelabLabel)) == std::string::npos) // not a onelab line
    outfile << line << std::endl; 
  else{ 
    if ( (pos=line.find(param)) != std::string::npos) {// onelab.param
      //skip the line
    }
    else if ( (pos=line.find(include)) != std::string::npos) {// onelab.include
      cursor = pos+include.length();
      pos=line.find_first_of(')',cursor)+1;
      if(enclosed(line.substr(cursor,pos-cursor),arguments)<1)
	Msg::Fatal("ONELAB misformed <%s> statement: (%s)",include.c_str(),line.c_str());
      convert_onefile(arguments[0],outfile);
    }
    else if ( (pos=line.find(iftrue)) != std::string::npos) {// onelab.iftrue
      cursor = pos+iftrue.length();
      pos=line.find_first_of(')',cursor)+1;
      if(enclosed(line.substr(cursor,pos-cursor),arguments)<1)
	Msg::Fatal("ONELAB misformed <%s> statement: (%s)",iftrue.c_str(),line.c_str());
      bool condition = false; 
     if((it = _parameters.find(arguments[0])) == _parameters.end()) // find complete name inclusive path
       Msg::Fatal("ONELAB %s: unknown variable: %s",iftrue.c_str(),arguments[0].c_str());
      get(numbers,*it);
      if (numbers.size())
	condition = (bool) numbers[0].getValue();
      if (!convert_ifstatement(infile,outfile,condition))
	Msg::Fatal("ONELAB misformed <%s> statement: %s",iftrue.c_str(),arguments[0].c_str());     
    }
    else if ( (pos=line.find(ifequal)) != std::string::npos) {// onelab.ifequal
      cursor = pos+ifequal.length();
      pos=line.find_first_of(')',cursor)+1;
      if(enclosed(line.substr(cursor,pos-cursor),arguments)<2)
	Msg::Fatal("ONELAB misformed <%s> statement: (%s)",iftrue.c_str(),line.c_str());;
      bool condition= false;
      if((it = _parameters.find(arguments[0])) == _parameters.end()) // find complete name inclusive path
	Msg::Fatal("ONELAB %s: unknown variable: %s",ifequal.c_str(),arguments[0].c_str());
      get(strings,*it);
      if (strings.size())
	condition =  !strings[0].getValue().compare(arguments[1]);
      if (!convert_ifstatement(infile,outfile,condition))
	Msg::Fatal("ONELAB misformed <%s> statement: (%s)",ifequal.c_str(),line.c_str());
    }
    else if ( (pos=line.find(getValue)) != std::string::npos) {// onelab.getValue
      // onelab.getValue, possibly several times in the line
      cursor=0;
      while ( (pos=line.find(getValue,cursor)) != std::string::npos){
	pos0=pos; // for further use
	cursor = pos+getValue.length();
	pos=line.find_first_of(')',cursor)+1;
	if(enclosed(line.substr(cursor,pos-cursor),arguments)<1)
	  Msg::Fatal("ONELAB misformed <%s> statement: (%s)",getValue.c_str(),line.c_str());
	
	if((it = _parameters.find(arguments[0])) == _parameters.end()) // find complete name inclusive path
	  Msg::Fatal("ONELAB %s: unknown variable: %s",getValue.c_str(),arguments[0].c_str());
	std::string paramName;
	paramName.assign(*it); 
	std::cout << "getValue:<" << arguments[0] << "> => <" << paramName << ">" << std::endl;
	get(numbers,paramName);
	if (numbers.size()){
	  std::stringstream Num;
	  Num << numbers[0].getValue();
	  buff.assign(Num.str());
	}
	else{
	  get(strings,paramName);
	  if (strings.size())
	    buff.assign(strings[0].getValue());
	  else
	    Msg::Fatal("ONELAB unknown variable: %s",paramName.c_str());
	}
	line.replace(pos0,pos-pos0,buff); 
	cursor=pos0+buff.length();
      }
      outfile << line << std::endl; 
    }
    else
      Msg::Fatal("ONELAB syntax error: %s",line.c_str());
  }
}

bool InterfacedClient::convert_onefile(std::string ifilename, std::ofstream &outfile) { 
  std::vector<onelab::number> numbers;
  std::vector<onelab::string> strings;
  int pos0,pos,cursor;
  char sep=';';
  std::string line,buff;
  std::ifstream infile(ifilename.c_str());

  if (infile.is_open()){
    while ( infile.good() ) {
      getline (infile,line);
      convert_oneline(line,infile,outfile);
    }
    infile.close();
  }
  else
    Msg::Fatal("The file %s cannot be opened",ifilename.c_str());
}

void InterfacedClient::setFileName(const std::string &fileName) { 
  if(fileName.empty())
    Msg::Fatal("No valid input file given for client <%s>.",getName().c_str());
  else{
    _fileName.assign(fileName+_extension); 
    //checkIfPresent(_fileName); not yet we shoild look for .ext_onelab
  }
}

void InterfacedClient::initialize() {
  Msg::SetOnelabString(getName() + "/Action","initialize"); // a titre indicatif
}

void InterfacedClient::analyze() { 
  Msg::SetOnelabString(getName() + "/Action","check"); // a titre indicatif
  std::string ifilename = _fileName + "_onelab";
  checkIfPresent(ifilename);
  analyze_onefile(ifilename); // recursive
}

void InterfacedClient::convert() { 
  std::string ifilename = _fileName + "_onelab";
  checkIfPresent(ifilename);
  std::string ofilename = _fileName ;
  std::ofstream outfile(ofilename.c_str());

  if (outfile.is_open())
    convert_onefile(ifilename,outfile);
  else
    Msg::Fatal("The file <%s> cannot be opened",ofilename.c_str());
  outfile.close();
  checkIfModified(ofilename);
}

void InterfacedClient::compute() { 
  convert();
  Msg::SetOnelabString(getName() + "/Action","compute"); // a titre indicatif
  std::string commandLine = _commandLine + " " + _fileName ;
  commandLine.append(" " + _options);
  //commandLine.append(" &> " + _name + ".log");
  Msg::Info("Client %s launched",_name.c_str());
  std::cout << "Commandline:" << commandLine.c_str() << std::endl;
  if ( int error = system(commandLine.c_str())) { 
    Msg::Error("Client %s returned error %d",_name.c_str(),error);
  }
  Msg::Info("Client %s completed",_name.c_str());
}

// ENCAPSULATED Client

void EncapsulatedClient::initialize() {
  set(onelab::string(getName()+"/Action", "initialize"));
  onelab::server::citer it= onelab::server::instance()->findClient(getName());
  onelab::client *c = it->second;
  c->run();
}

void EncapsulatedClient::analyze() {
  set(onelab::string(getName()+"/Action", "check"));
  onelab::server::citer it= onelab::server::instance()->findClient(getName());
  onelab::client *c = it->second;
  c->run();
}

void EncapsulatedClient::compute() {
  set(onelab::string(getName()+"/Action", "compute"));
  onelab::server::citer it= onelab::server::instance()->findClient(getName());
  onelab::client *c = it->second;
  c->run();
}

void EncapsulatedClient::setFileName(const std::string &fileName) {
  if(fileName.empty())
    Msg::Fatal("No valid input file given for client <%s>.",getName().c_str());
  else{
    set(onelab::string(getName()+"/1ModelName", fileName + getExtension()));
    checkIfPresent(getFileName());
  }
}
void EncapsulatedClient::setLineOptions(const std::string &options) {
  if(options.size())
    set(onelab::string(getName()+"/9LineOptions", options));
}
std::string EncapsulatedClient::getFileName() {
  return Msg::GetOnelabString( getName() + "/1ModelName");
}


/*
ONELAB additional tools (no access to server in tools
 */

// utilisé pour le main() des métamodèles

int getOptions(int argc, char *argv[], std::string &action, std::string &commandLine, std::string &fileName, std::string &clientName, std::string &sockName, int &modelNumber){

  commandLine=argv[0];
  action="compute";
  int i= 1;
  while(i < argc) {
    if(argv[i][0] == '-') {
      if(!strcmp(argv[i] + 1, "m")) {
	i++;
	modelNumber = (atoi(argv[i]));
        i++;
      }
      else if(!strcmp(argv[i] + 1, "onelab")) {
	i++;
	clientName = argv[i];
        i++;
	sockName = argv[i];
        i++;
      }
      else if(!strcmp(argv[i] + 1, "c")) {
	i++;
	std::cout << argv[0] << " has " << onelab::server::instance()->getNumClients() << " clients\n" ;
	for(onelab::server::citer it = onelab::server::instance()->firstClient();
	    it != onelab::server::instance()->lastClient(); it++){
	  std::cout << it->second->getId() << ':' << it->second->getName() << "/" << std::endl;
	}
	exit(1);
      }
      else if(!strcmp(argv[i] + 1, "a")) {
	i++;
	action="check";
      }
      else {
	i++;
	printf("Usage: %s [-m num -a -c]\n", argv[0]);
	printf("Options are:\nm      model number\n");
	printf("h      displays this help\n");
	printf("a      analyze only\n");
	printf("c      list of clients\n");
	//exit(1);
      }
    }
    else{
      fileName=argv[i];
      i++;
    }
  }
}

std::string itoa(const int i){
  std::ostringstream tmp;
  tmp << i ;
  return tmp.str();
}

int onelab_step;
int newStep(){
  if (onelab_step==0){
    system("echo 'start' > onelab.log");
    system("touch onelab.start; touch onelab.progress");
  }
  system("find . -newer onelab.progress > onelab.modified");
  system("touch onelab.progress");
  onelab_step++;
}
int getStep(){
  return onelab_step;
}


#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
std::string getUserHomedir(){
  struct passwd *pw = getpwuid(getuid());
  std::string str(pw->pw_dir);
  str.append("/");
  return str;
}

#include <sys/param.h>
std::string getCurrentWorkdir(){
  char path[MAXPATHLEN];
  getcwd(path, MAXPATHLEN);
  std::string str = path;
  return str;
}

#include <sys/stat.h>		
#include <ctime>

bool fileExist(std::string filename){
  struct stat buf;
  if(!stat(filename.c_str(), &buf)){
    std::string cmd = "touch " + filename;
    system(cmd.c_str());
    return true;
  }
  else
    return false;
}

bool checkIfPresent(std::string filename){
  struct stat buf;
  if (!stat(filename.c_str(), &buf))
    return true;
  else{
    Msg::Fatal("The file %s is not present",filename.c_str());
    return false;
  }
}
bool checkIfModified(std::string filename){
  struct stat buf1,buf2;
  if (stat("onelab.start", &buf1))
    Msg::Fatal("The file %s does not exist.","onelab.start");
  if (stat(filename.c_str(), &buf2))
    Msg::Fatal("The file %s does not exist.",filename.c_str());
  if (difftime(buf1.st_mtime, buf2.st_mtime) > 0)
    Msg::Fatal("The file %s has not been modified.",filename.c_str());
  return true;
}

int systemCall(std::string cmd){
  printf("ONELAB System call(%s)\n", cmd.c_str());
  return system(cmd.c_str()); 
}

void GmshDisplay(onelab::remoteNetworkClient *loader, std::string fileName, std::vector<std::string> choices){
  bool hasGmsh=false;
  if(choices.empty()) return;
  if(loader)
    hasGmsh=loader->getName().compare("MetaModel");
  std::string cmd="gmsh " + fileName + ".geo ";
  for(unsigned int i = 0; i < choices.size(); i++){
    cmd.append(choices[i]+" ");
    checkIfModified(choices[i]);
    if(hasGmsh)
      loader->sendMergeFileRequest(choices[i]);
  }
  std::cout << "cmd=<" << cmd << ">" << std::endl;
  if(!hasGmsh) systemCall(cmd);
}
void GmshDisplay(onelab::remoteNetworkClient *loader, std::string modelName, std::string fileName){
  checkIfModified(fileName);
  if(loader)
    loader->sendMergeFileRequest(fileName);
  else {
    std::string cmd= "gmsh " + modelName + ".geo " + fileName;
    systemCall(cmd);
  }
}


void appendOption(std::string &str, const std::string &what, const int val){
  if(val){ // assumes val=0 is the default
    std::stringstream Num;
    Num << val;
    str.append( what + " " + Num.str() + " ");
  }
}
void appendOption(std::string &str, const std::string &what){
  str.append( what + " ");
}

std::vector <double> extract_column(const int col, array data){
  std::vector<double> column;
  for ( int i=0; i<data.size(); i++)
    if (  col>0 && col<=data[i].size())
      column.push_back(data[i][col-1]);
    else
      Msg::Fatal("Column number (%d) out of range.",col);
  return column;
}

double find_in_array(const int lin, const int col, const std::vector <std::vector <double> > &data){
  if ( lin>=0 && lin<data.size())
    if (  col>=0 && col<data[lin-1].size())
      return data[lin-1][col-1];
  Msg::Fatal("The value has not been calculated: (%d,%d) out of range",lin,col);
}

array read_array(std::string filename, char sep){
  std::ifstream infile(filename.c_str());
  std::vector <std::vector <double> > array;

  while (infile){
    std::string s;
    if (!getline( infile, s )) break;
    std::istringstream ss( s );
    std::vector <double> record;
    while (ss){
      std::string s;
      if (!getline( ss, s, sep )) break;
      if ( s.size() ){
	//std::cout << "Read=<" << s << ">" << std::endl;
	record.push_back( atof( s.c_str() ));
      }
    }
    array.push_back( record );
  }
  if (!infile.eof()){
    std::cerr << "Fooey!\n";
  }
  return array;
}

  
