//#######################################################################################################
//#################################### Plugin 210: MQTT Import ##########################################
//#################################### Version 0.3 12-May-2016 ##########################################
//#######################################################################################################

// This simple task reads data from the MQTT Import  input stream and saves the value

#define PLUGIN_210
#define PLUGIN_ID_210         210
#define PLUGIN_NAME_210       "MQTT Import"

#define PLUGIN_VALUENAME1_210 "Value1"
#define PLUGIN_VALUENAME2_210 "Value2"
#define PLUGIN_VALUENAME3_210 "Value3"
#define PLUGIN_VALUENAME4_210 "Value4"

#define PLUGIN_IMPORT 210		// This is a 'private' function used only by this import module

// The line below defines the dummy function PLUGIN_COMMAND which is only for Namirda use

#ifndef PLUGIN_COMMAND
#define PLUGIN_COMMAND 999
#endif    

// Define MQTT client for this plugin only

PubSubClient MQTTclient_210("");		// Create a new pubsub instance

boolean Plugin_210(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  char deviceTemplate[4][41];		// variable for saving the subscription topics
  
  //
  // Generate the MQTT import client name from the system name and a suffix
  //
  String tmpClientName = "%sysname%-Import";
  String ClientName = parseTemplate(tmpClientName, 20);

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_210;
        Device[deviceCount].Type = SENSOR_TYPE_SWITCH;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;     // This means it has a single pin
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true; // Need this in order to get the decimals option
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption=false;     
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_210);
        break;
      }
      
    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_210));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_210));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_210));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_210));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {

        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        for (byte varNr = 0; varNr < 4; varNr++)
        {
          string += F("<TR><TD>MQTT Topic ");
          string += varNr + 1;
          string += F(":<TD><input type='text' size='40' maxlength='40' name='Plugin_210_template");
          string += varNr + 1;
          string += F("' value='");
          string += deviceTemplate[varNr];
          string += F("'>");
        }
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
		String argName;

        for (byte varNr = 0; varNr < 4; varNr++)
        {
          argName = F("Plugin_210_template");
          argName += varNr + 1;
          strncpy(deviceTemplate[varNr], WebServer.arg(argName).c_str(), sizeof(deviceTemplate[varNr]));
		}

        Settings.TaskDeviceID[event->TaskIndex] = 1; // temp fix, needs a dummy value

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        success = true;
        break;
      }

    case PLUGIN_INIT:
      {

//    When we edit the subscription data from the webserver, the plugin is called again with init.
//    In order to resubscribe we have to disconnect and reconnect in order to get rid of any obsolete subscriptions

        MQTTclient_210.disconnect();

		if (MQTTConnect_210(ClientName))
		{
//		Subscribe to ALL the topics from ALL instance of this import module
			MQTTsubscribe();
			success = true;
		}
		else 
		{
			success=false;
        }
      }

    case PLUGIN_TEN_PER_SECOND:
      {
        MQTTclient_210.loop();		// Listen out for callbacks
        success=true;
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
//  Here we check that the MQTT client is alive. 

          if (!MQTTclient_210.connected()){
          
            String log = F("CON  : MQTT 210 Connection lost");
            addLog(LOG_LEVEL_ERROR, log);
              
            MQTTclient_210.disconnect();
            delay(1000);
              
            if (! MQTTConnect_210(ClientName)){
              success=false;
              break;
            }

            MQTTsubscribe();
          }     
        
          success=true;
          break;
      }

    case PLUGIN_READ:
      {
        // This routine does not output any data and so we do not need to respond to regular read requests
        success = false;
        break;
      }

    case PLUGIN_COMMAND:
      {
        // This option is called when user has made a request to this task
        
//      Get the payload

        String Payload=event->String2;
        float floatPayload=string2float(Payload);

//      Abort if not a valid number

        if (floatPayload == -999)
        {
          String log=F("ERR  : Illegal value for Payload ");
          log+=Payload;
          log += " - must be a valid floating point number";
          addLog(LOG_LEVEL_INFO,log);
          break;
        }
        
//      Get the valuenameindex for this command
        
        byte ValueNameIndex=getValueNameIndex(event->TaskIndex,string);
        
        if (ValueNameIndex == 255){
          addLog(LOG_LEVEL_INFO,"ERR  : Internal Error");
          break;
        }
        
        UserVar[event->BaseVarIndex+ValueNameIndex]=floatPayload;  // Save the new value

        logUpdates(210,event->TaskIndex,ValueNameIndex,floatPayload);

        success=true;
        break;
    }
  

    case PLUGIN_IMPORT:
      {
        // This is a private option only used by the MQTT 210 callback function

//      Get the payload and check it out

        String Payload=event->String2;
        float floatPayload=string2float(Payload);
        
        LoadTaskSettings(event->TaskIndex);
                     
        if (floatPayload == -999){
          String log=F("ERR  : Bad Import MQTT Command ");
          log+=event->String1;
          addLog(LOG_LEVEL_ERROR,log);
          log="ERR  : Illegal Payload ";
          log+=Payload;
          log+="  ";
          log+=ExtraTaskSettings.TaskDeviceName;
          addLog(LOG_LEVEL_INFO,log);
          success=false;
          break;
        }

//      Get the Topic and see if it matches any of the subscriptions

        String Topic=event->String1;

        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));

        for (byte x = 0; x < 4; x++)
        {
            String subscriptionTopic = deviceTemplate[x];
			subscriptionTopic.trim();
			if (subscriptionTopic.length() == 0) continue;							// skip blank subscriptions

			// Now check if the incoming topic matches one of our subscriptions

            if (MQTTCheckSubscription(Topic,subscriptionTopic))
			{	
              UserVar[event->BaseVarIndex+x]=floatPayload;							// Save the new value
              logUpdates(210,event->TaskIndex,x,UserVar[event->BaseVarIndex+x]);    // Log the event

			  // Generate event for rules processing - proposed by TridentTD
			  
			  if (Settings.UseRules)
			  {
				  String event = F("");
				  event += ExtraTaskSettings.TaskDeviceName;
				  event += "#";
				  event += ExtraTaskSettings.TaskDeviceValueNames[x];
				  event += "=";
				  event += floatPayload;
				  rulesProcessing(event);
			  }
            }
        }

        break;
           
    }
  }

  return success;
}

//
//	Subscribe to the topics requested by ALL calls to this plugin
//
void MQTTsubscribe(){

// Subscribe to the topics requested by ALL calls to this plugin.
// We do this because if the connection to the broker is lost, we want to resubscribe for all instances.

  char deviceTemplate[4][41];

//	Loop over all tasks looking for a 210 instance

  for (byte y = 0; y < TASKS_MAX; y++){

    if (Settings.TaskDeviceNumber[y] == 210) {
      LoadCustomTaskSettings(y, (byte*)&deviceTemplate, sizeof(deviceTemplate));

	  // Now loop over all import variables and subscribe to those that are not blank

      for (byte x = 0; x < 4; x++) {
        String subscribeTo = deviceTemplate[x];
        if (subscribeTo.length() > 0 ){
              
           MQTTclient_210.subscribe(subscribeTo);

           String log=F("SUB  : [");
           LoadTaskSettings(y);
           log+=ExtraTaskSettings.TaskDeviceName;
           log+="#";
           log+=ExtraTaskSettings.TaskDeviceValueNames[x];
           log+="] subscribed to ";
           log+=subscribeTo;
           addLog(LOG_LEVEL_INFO,log);
        }
      }
    }
  }
}
//
// handle MQTT messages
//
void callback_210(const MQTT::Publish& pub) {

// Here we have incomng MQTT messages from the mqtt import module

  String topic = pub.topic();
  String payload = pub.payload_string();
  
  byte DeviceIndex=getDeviceIndex(210);   // This is the device index of 210 modules -there should be one!
  
  // We generate a temp event structure to pass to the plugins
  
  struct EventStruct TempEvent;

  TempEvent.String1=topic;                            // This is the topic of the message
  TempEvent.String2=payload;                          // This is the payload

//  Here we loop over all tasks and call each 210 plugin with function PLUGIN_IMPORT

  for (byte y = 0; y < TASKS_MAX; y++)
  {
      if (Settings.TaskDeviceNumber[y] == 210){                 // if we have found a 210 device, then give it something to think about!
            TempEvent.TaskIndex=y;
            TempEvent.BaseVarIndex=y * VARS_PER_TASK;           // This is the index in Uservar where values for this task are stored
            Plugin_ptr[DeviceIndex](PLUGIN_IMPORT, &TempEvent, payload);
      }      
  }
}

//
// Make a new client connection to the mqtt broker
//
boolean MQTTConnect_210(String clientid)

{
  boolean result=false;

// Do nothing if already connected

  if (MQTTclient_210.connected()){
    result=true;
    return result;
  }
  
  IPAddress MQTTBrokerIP(Settings.Controller_IP);
  MQTTclient_210.set_server(MQTTBrokerIP, Settings.ControllerPort);
  MQTTclient_210.set_callback(callback_210);

//  Try three times for a connection

  for (byte x = 1; x < 3; x++)
  {
    String log = "";

    if ((SecuritySettings.ControllerUser[0] != 0) && (SecuritySettings.ControllerPassword[0] != 0))
      result = (MQTTclient_210.connect(MQTT::Connect(clientid).set_auth(SecuritySettings.ControllerUser, SecuritySettings.ControllerPassword)));
    else
      result = (MQTTclient_210.connect(clientid));

    if (result)
    {
      log = F("CON  : Import MQTT Connected to broker with Client ID=");
	  log += clientid;
      addLog(LOG_LEVEL_INFO, log);
      
      break; // end loop if succesfull
    }
    else
    {
      log = F("CON  : Import MQTT Failed to connect to broker");
      addLog(LOG_LEVEL_ERROR, log);
    }

    delay(500);
  }
  return result;
}
//
// Check to see if Topic matches the MQTT subscription
//
boolean MQTTCheckSubscription(String Topic, String Subscription) {

	String tmpTopic = Topic;
	String tmpSub = Subscription;

	tmpTopic.trim();
	tmpSub.trim();

	// Get rid of any initial /

	if (tmpTopic.substring(0, 1) == "/")tmpTopic = tmpTopic.substring(1);
	if (tmpSub.substring(0, 1) == "/")tmpSub = tmpSub.substring(1);

	// Add trailing / if required

	int lenTopic = tmpTopic.length();
	if (tmpTopic.substring(lenTopic - 1, lenTopic) != "/")tmpTopic += "/";

	int lenSub = tmpSub.length();
	if (tmpSub.substring(lenSub - 1, lenSub) != "/")tmpSub += "/";

	// Now get first part

	int SlashTopic;
	int SlashSub;
	int count = 0;

	String pTopic;
	String pSub;

	while (count < 10) {

		//  Get locations of the first /

		SlashTopic = tmpTopic.indexOf('/');
		SlashSub = tmpSub.indexOf('/');

		//  If no slashes found then match is OK
		//  If only one slash found then not OK

		if ((SlashTopic == -1) && (SlashSub == -1)) return true;
		if ((SlashTopic == -1) && (SlashSub != -1)) return false;
		if ((SlashTopic != -1) && (SlashSub == -1)) return false;

		//  Get the values for the current subtopic

		pTopic = tmpTopic.substring(0, SlashTopic);
		pSub = tmpSub.substring(0, SlashSub);

		//  And strip the subtopic from the topic

		tmpTopic = tmpTopic.substring(SlashTopic + 1);
		tmpSub = tmpSub.substring(SlashSub + 1);

		//  If the subtopics match then OK - otherwise fail
		if (pSub == "#")  return true;
		if ((pTopic != pSub) && (pSub != "+"))return false;

		count = count + 1;
	}
	return false;
}



