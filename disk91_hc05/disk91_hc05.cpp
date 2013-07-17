/* **********************************************************
 * Disk91_HC05 library Header
 * **********************************************************
 * This source is under GNU GPL
 * Any information : contact me on www.disk91.com
 * ----------------------------------------------------------
 * Libary size with debug elements about 13Kb w/o debug : 10 Kb
 * To setup the library, please configure the following #define
 * DEBUG => when set by #define DEBUG - activate logs
 * blueToothSerial => link the HC05 with the arduino Serial port choosen.
 * **********************************************************
 * THE disk91_HC05 SOFTWARE IS PROVIDED TO YOU "AS IS," AND WE MAKE NO EXPRESS 
 * OR IMPLIED WARRANTIES WHATSOEVER WITH RESPECT TO ITS FUNCTIONALITY, OPERABILITY,
 * OR USE, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR INFRINGEMENT. 
 * WE EXPRESSLY DISCLAIM ANY LIABILITY WHATSOEVER FOR ANY DIRECT, INDIRECT,
 * CONSEQUENTIAL, INCIDENTAL OR SPECIAL DAMAGES, INCLUDING, WITHOUT LIMITATION,
 * LOST REVENUES, LOST PROFITS, LOSSES RESULTING FROM BUSINESS INTERRUPTION OR 
 * LOSS OF DATA, REGARDLESS OF THE FORM OF ACTION OR LEGAL THEORY UNDER WHICH 
 * THE LIABILITY MAY BE ASSERTED, EVEN IF ADVISED OF THE POSSIBILITY OR LIKELIHOOD 
 * OF SUCH DAMAGES.
 * **********************************************************
 */
#include "disk91_hc05.h"
 
Disk91_hc05::Disk91_hc05()
{
   rates[0] = 38400; 
   rates[1] = 38400; 
   rates[2] = 115200;
   rates[3] = 115200;
   rates[4] = 9600;  
   rates[5] = 19200; 
   rates[6] = 57600; 
   detected_addressN=0;
   forcedState=ST_NOFORCE;
}

/* --- Setup Bluetooth HC-05 device to be ready for inquiery
 * Set device name as BT_CAM, passwd 0000, inquiery time 10s to 1 device
 */
boolean Disk91_hc05::setupBlueToothConnection(char * devName)
{
  char _buffer[128];

  print_debug("Entering setupBlueToothConnection() ");
  bootup = true;              // device is booting
  reqPairing = ( getHC05ADCN() == 0 ) ;  // if no device is already paired, request pairing.
  initSuccess = false;

  if ( getHC05Connection() ) {    
     if ( getHC05State() != 0 )  {
        sendHC05AtCmd("AT+RESET", true);          // If not in state initialized : reset
        delay(5000);
     }     
     if ( getHC05State() == 0 )  {
        sendHC05AtCmd("AT+INIT",true);           // Init SPP
        delay(2000);
        strcpy(_buffer,"AT+NAME=");
        strcat(_buffer,devName);
        sendHC05AtCmd(_buffer,true);    // BT displayed name
        sendHC05AtCmd("AT+PSWD=0000",true);      // pairing password is 0000
        sendHC05AtCmd("AT+UART=38400,0,0",true); // serial over BT rate + Arduino rate (next restart)
        sendHC05AtCmd("AT+IAC=9e8b33",true);     // use a Password for pairing
        sendHC05AtCmd("AT+CMODE=1",true);        // connect to any address
        initSuccess = true;
     }     
  } else {
    print_debug("Leaving setupBlueToothConnection() with error");
    return false;
  }
  print_debug("Leaving setupBlueToothConnection() ");
  return true;
}


/* -------------------------------------------------------------
 * Establsh connection to a device. return only when the connection
 * has been done with true
 * --------------------------------------------------------------
 */
boolean Disk91_hc05::connect() 
{
 char buf[BUFSZ];
 int bufd;
 char recvChar,lrecvChar;
 int state;
 int time=0;

 if ( ! initSuccess ) return false;
 
 while (1) {
   state=getHC05State();
   print_debug2("HC05 State : ",state);
   switch ( state ) {
      case ST_INITIALIZED:
           // On startup, try to connect to existing device, then go for pairing if failed
           if ( reqPairing ) {
              reqPairing = false;
              // Start a pairing process
              forceHC05State(ST_SEARCH_FOR_PAIR);
              time=0;
           } else {
              if ( getHC05ADCN() > 0 ) 
                  forceHC05State(ST_PAIRED);
              // mode sleep à travailler
              delay(5000); 
           }
           break;           
             
      // Not a real existing case, just to simplify algotithm
      case ST_SEARCH_FOR_PAIR:    
           if ( time == 0 ) {
               // for the first minute we can start to INQUIRE if someone whants to pair with us as a salve
               sendHC05AtCmd("AT+ROLE=0",true);       // salve mode
               sendHC05AtCmd("AT+CLASS=40620",true);  // this bt device type camera
               sendHC05AtCmd("AT+INQ",true);          // Start INQUIERING => change state to pairable  
               print_debug("HC05 waiting for pairing as a slave");
               bootup=false;			      // to not re-enter ...
           } 
           if ( time < MAX_SLAVE_TIME ) {
               print_debug2("HC05 is inquiering as slave - ",time);
               // Start inquiering ... this will change the status for PAIRABLE but we will proceed here
               // At this point if OK is read it means that we are connected with the device ... Inquiery success and finished with +DISC
               // already set : blueToothSerial.setTimeout(200);
               bufd = blueToothSerial.readBytes(buf,BUFSZ);
               if (bufd >= 2) {
                 if ( buf[0] == 'O' && buf[1] == 'K' ) {
                   forceHC05State(ST_PAIRED);
                 }
               }
               delay(1000);
               time++;      
           } else {
               print_debug("HC05 pairing as master");
               // At the end of the enquiering delay, start to inquier in master mode ... 
               forceHC05State(ST_NOFORCE);               // Stop Slave inquiering ... reinit
               sendHC05AtCmd("AT+RESET", true);          // Reset
               delay(5000);
               sendHC05AtCmd("AT+INIT",true);            // Init SPP
               delay(2000);

               // Change mode to connect as a master
               sendHC05AtCmd("AT+ROLE=1",true);       // act as master
               sendHC05AtCmd("AT+CLASS=0",true);      // search for everything (use 200 for a smartphone)
               startHC05Inq(MAX_MASTER_TIME); 
               // Did we found something ?
               if (  detected_addressN > 0 ) {
                   // First try to connected to already known devices if we have
                   for ( int k = 0 ; k < detected_addressN ; k++ ) {
                     strcpy(buf,"AT+FSAD=");
                     if (sendHC05AtCmd(strcat(buf,detected_address[k]),true) < 0 ) {
                       // this address is already known ... linking
                       strcpy(buf,"AT+LINK=");
                       if (sendHC05AtCmd(strcat(buf,detected_address[k]),false) < 0 ) break;     //link sucess
                     }
                   }
                 
                   // Then try to pair to listed devices
                   for ( int k = 0 ; k < detected_addressN ; k++ ) {
                     strcpy(buf,"AT+FSAD=");
                     if ( sendHC05AtCmd(strcat(buf,detected_address[k]),true) == COD_FAIL ) {
                       strcpy(buf,"AT+PAIR=");
                       strcat(buf,detected_address[k]);
                       strcat(buf,",20");
                       if (sendHC05AtCmd(buf,false) < 0 ) {
                            // pairing  sucess
                            strcpy(buf,"AT+LINK=");
                            if (sendHC05AtCmd(strcat(buf,detected_address[k]),false) < 0 ) {
                              forceHC05State(ST_CONNECTED);
                              break;     //link sucess
                            }
                        }
                     }
                   }
                 }
                 forceHC05State(ST_NOFORCE);
           }
           break;

      case ST_PAIRABLE:
           break;
           
      case ST_INQUIERING:
           print_debug("HC05 is inquiering - invalid state");
           // reset
           forceHC05State(ST_NOFORCE);               // Stop Slave inquiering ... reinit
           sendHC05AtCmd("AT+RESET", true);          // Reset
           delay(5000);
           sendHC05AtCmd("AT+INIT",true);            // Init SPP
           delay(2000);
           break;
          
      case ST_PAIRED:
           print_debug("HC05 is paired to a device");
           if ( getHC05Mrad() ) {
               // Connect to the last device if possible
               strcpy(buf,"AT+LINK=");
               if (sendHC05AtCmd(strcat(buf,detected_address[0]),false) < 0 ) {
                 forceHC05State(ST_CONNECTED);
               }
               else {
		 // If connection not succeed, back to standard process ...
                 sendHC05AtCmd("AT",true); // it seems that after a AT+LINK the next AT command is not concidered
                
                 // When connection fail, if at bootup, we start a new pairing process
                 if ( bootup ) {
                    reqPairing = true;                  
                    forceHC05State(ST_NOFORCE); 
                 }
               }
           }
           bootup = false; // bootup period is finished after first pairing try
           break;
            
      case ST_DISCONNECTED:
           print_debug("Disconnection detected");
           sendHC05AtCmd("AT+RESET", true);          // If not in state initialized : reset
           delay(5000);
           sendHC05AtCmd("AT+INIT",true);            // Init SPP
           delay(2000);
           break;

      case ST_CONNECTED:
           print_debug("Device Connected");
           return true;


      default:
           delay(1000);
           break;
   }      
 }

}

/* ---------------------------------------------------------------
 * Receive string from bluetooth - not blocking operation
 * return number of char received
 * return -1 when the connection is broken
 * ---------------------------------------------------------------
 */
int Disk91_hc05::receive(char * buf, int maxsz) 
{
    int bufd;
    if ( getHC05State() != ST_CONNECTED )
      return -1;
      
    // We are waiting for +DISC command now on blueToothSerial to quit the connection mode
    bufd = blueToothSerial.readBytes(buf,BUFSZ);
    if (bufd > 0 ) {
       if (bufd > 5) {
          if ( buf[0] == '+' && buf[1] == 'D' && buf[2] == 'I' && buf[3] == 'S' && buf[4] == 'C' ) {
             // detect disconnection
             forceHC05State(ST_NOFORCE);
             return -1;
          } 
       }
       buf[bufd]=0;
     }
     return bufd;
}

/* ---------------------------------------------------------------
 * Send string over bluetooth - blocking operation
 * return false if disconnected
 * ---------------------------------------------------------------
 */
boolean Disk91_hc05::send(char * buf) 
{
    if ( getHC05State() == ST_CONNECTED ) 
       blueToothSerial.print(buf);
    else return false;
    return true;
}



/* ======================================================================
 * HC-05 Subroutine
 * ======================================================================
 */

/* --- Search for HC05 device and baudrate 
 * (peace of code from https://github.com/jdunmire/HC05 project)
 * send AT at different baudrate and expect OK
 * it seems that testing multiple time the same rate ensure to have a better detection
 * return true is found, false otherwise.
 */
boolean Disk91_hc05::getHC05Connection() {
  int numRates = sizeof(rates)/sizeof(unsigned long);
  int recvd = 0;
  char _buffer[128];
  int _bufsize = sizeof(_buffer)/sizeof(char);
    
  print_debug("Entering getHC05Connection()");
  for(int rn = 0; rn < numRates; rn++)
  {
    print_debug2(" * Trying new rate : ",rates[rn]);
    blueToothSerial.begin(rates[rn]);
    blueToothSerial.setTimeout(200);
    blueToothSerial.write("AT\r\n");
    delay(200);
    recvd = blueToothSerial.readBytes(_buffer,_bufsize);
    if (recvd > 0) {
      print_debug("Leaving getHC05Connection() - Found card ");
      return true;
    } 
  }
  print_debug("Leaving getHC05Connection() - Not Found ");
  return false;
}

/* --- Force a specific state as the HC05 does not commute as I could expect
 * Once the state is forced, the device is not anymore Inquiery for State until
 * this forced state is back to ST_ERROR
 */

void Disk91_hc05::forceHC05State(int state) {
   forcedState=state;
}

/* --- Get STATE return state code, according to:
 * -1 : ERROR            -2 : NOFORCE    -3 : SEARCH_FOR_PAIR
 *  0 : INITIALIZED      1 : READY        2 : PAIRABLE    3 : PAIRED
 *  4 : INQUIRING        5 : CONNECTING   6 : CONNECTED   7 : DISCONNECTED
 *  8 : NUKNOW
 */
int Disk91_hc05::getHC05State() {
  int recvd = 0;
  char _buffer[128];
  int _bufsize = sizeof(_buffer)/sizeof(char);
  int ret=ST_ERROR;

   print_debug("Entering getHC05State()");
   if (forcedState == ST_NOFORCE ) {
   
     blueToothSerial.print("AT+STATE?\r\n"); 
     delay(200);
     recvd = blueToothSerial.readBytes(_buffer,_bufsize);
     if (recvd > 2 ) {
         int i = 0;
         while ( i < _bufsize-1 ) {
            if ( _buffer[i] == '\n' ) { _buffer[i] = 0 ; i = _bufsize; }
            else i++;
         }
         print_debug2(" * State Received :",_buffer);
         if ( recvd > 12 ) {
           switch (_buffer[7]) {
             case 'R' : ret= 1; break; 
             case 'D' : ret= 7; break;
             case 'I' : ret= (_buffer[9]=='I')?0:4; break;
             case 'P' : ret= (_buffer[11]=='A')?2:3; break;
             case 'C' : if ( recvd > 16 ) ret = ( _buffer[14] == 'I')?5:6; else ret= ST_ERROR; break;    
             default : ret = ST_ERROR; break;
           }
         }
     } else ret= ST_ERROR;
   } else {
     ret = forcedState; 
   }
   // Other cases : consider as valid command
   print_debug2("Leaving getHC05State() with ret code : ",ret);
   return ret;   
} 


/* --- Start Inquiering
 * start inquering, results are read from the serial line
 * timeout is set
 * return number of results
 */
int Disk91_hc05::startHC05Inq(int timeout ) {
  int recvd = 0;
  char _buffer[512];
  int _bufsize = sizeof(_buffer)/sizeof(char);

   print_debug("Entering startHC05Inq()");
   sprintf(_buffer,"AT+INQM=1,4,%d",timeout); // mode rssi, 4 device max, timeout*1.28s max
   sendHC05AtCmd(_buffer,true);
   blueToothSerial.print("AT+INQ\r\n");           // start INQ                     
   delay(1300*timeout);
   recvd = blueToothSerial.readBytes(_buffer,_bufsize);
   sendHC05AtCmd("AT+INQC",true);                  // Retour etat INITIALIZED
   blueToothSerial.flush();
   // Search for addresses in format : +INQ:aaaa:aa:aaaa,ttttt,ppppp 
   detected_addressN = 0;
   int i=0, v;
   while ( i < recvd ) {
     if ( recvd - i > 15 ) { // do not spend time looking for address is less than 15 byte
       if ( _buffer[i] == '+'  && _buffer[i+1] == 'I' && _buffer[i+2] == 'N' && _buffer[i+3] == 'Q' && _buffer[i+4] == ':' ) {
         // we have detected a response, search for separator (,)
         v=i+5;
         while ( v < recvd && _buffer[v] != ',' ) v++;
         if ( v < recvd ) { 
           memcpy(detected_address[detected_addressN],&_buffer[i+5], v-(i+5));  // copy address bloc
           detected_address[detected_addressN][v-(i+5)+1]=0;                    // add end of line
           
           for (int k=0 ; detected_address[detected_addressN][k] != 0 ; k++ )   // Change ':' separator by ',' as it will be this syntaxe latter used
              if ( detected_address[detected_addressN][k] == ':' ) detected_address[detected_addressN][k] = ',';
              
           int c=0;                                                             // we should count already existing occurence of this address
           for ( int j=0 ; j < detected_addressN ; j++ ) 
             if ( strcmp(detected_address[j],detected_address[detected_addressN]) == 0 ) c++;
           if ( c == 0 ) {
              print_debug2("Address found : ",detected_address[detected_addressN]);
              getHC05RName(detected_address[detected_addressN]);
              detected_addressN++;
              if (detected_addressN == 4 ) return 4;                            // No place left on table, skip
           }
           i=v;                                                                 // goto next
         }
       }
     }
     i++;
   }
   print_debug2("Leaving startHC05Inq() with ret code : ",detected_addressN);
   return detected_addressN;   
 } 

/* --- Get MRAD - Most Recent Used Address
 * return it in detected_Address[0], empty this table if not failed
 */
 boolean Disk91_hc05::getHC05Mrad() {
   int recvd = 0;
   char _buffer[128];
   int _bufsize = sizeof(_buffer)/sizeof(char);

   print_debug("Entering getHC05Mrad()");
   
   blueToothSerial.print("AT+MRAD?\r\n");
   delay(200);
   recvd = blueToothSerial.readBytes(_buffer,_bufsize);
   if (recvd > 6 ) {
       int i = 0;
       while ( i < _bufsize-1 ) {
          if ( _buffer[i] == '\n' ) { _buffer[i] = 0 ; i = _bufsize; }
          else i++;
       }
       print_debug2(" * Received :",_buffer);
       if ( _buffer[0]=='+' && _buffer[1] == 'M' ) {
         int v=6;
         while ( v < recvd && _buffer[v] != 0 ) v++;
         if ( v < recvd ) {
            memcpy(detected_address[0],&_buffer[6], v-6);                        // copy address bloc
            detected_address[0][v-6+1]=0;                        // add end of line
           
            for (int k=0 ; detected_address[0][k] != 0 ; k++ )   // Change ':' separator by ',' as it will be this syntaxe latter used
              if ( detected_address[0][k] == ':' ) detected_address[0][k] = ',';
              
            detected_addressN = 1;
            print_debug("Leaving getHC05Mrad() - true");
            return true;
         }        
       }
   }
   // Other cases : consider as valid command
   print_debug("Leaving getHC05Mrad() - false");
   return false;
}
    
 
/* --- Get ADCN value
 * ADCN is the number of paired devices stored in the memory
 * return the number from response format : +ADCN:X where X is the value on 1 or 2 digit
 * return -1 when error
 */
int Disk91_hc05::getHC05ADCN(){
  int recvd = 0;
  char _buffer[128];
  int _bufsize = sizeof(_buffer)/sizeof(char);
  int ret=-1;

   print_debug("Entering getHC05ADCN()");
   blueToothSerial.print("AT+ADCN?\r\n"); 
   delay(200);
   recvd = blueToothSerial.readBytes(_buffer,_bufsize);
   if (recvd > 7 ) { // test valid for 2 digit or 1 digit + \n
      if ( _buffer[0] == '+' ) {
         ret = _buffer[6] - '0'; 
         if ( _buffer[7] >= '0' && _buffer[7] <= '9' )
           ret = 10*ret + _buffer[7]-'0';
      }   
      print_debug2(" * ADCN :",ret);
   } else ret= -1;
   // Other cases : consider as valid command
   print_debug2("Leaving getHC05ADCN() with ret code : ",ret);
   return ret;   
} 
 
/* --- Send AT Command and get Error code back
 * imediate : when true directly read result, do not wait them to come
 *            make sense when you are not waiting for a remote device answer
 *
 * Send AT command over blueToothSerial then return -1 id OK error code otherwise
 * According to Doc
 * 0 - AT CMD Error           1 - Default result      2 - PSKEY write error      3 - Too Long
 * 4 - No Dev Name            5 - Bt NAP too long     6 - BT UAP too long        7 - BT LAP too long
 * 8 - No PIO num mask        9 - No PIO Num          10 - No BT device         11 - Too lenght of device
 * 12 - No Inquire ac        13 - Too long inq ac     14 - Invalid inq ac code  15 - Passkay Lenght is 0
 * 16 - Passkey len too long 17 - Invalide modul role 18 - Invalid baud rate    19 - Invalid strop bit
 * 20 - Invalid parity bit   21 - auth dev not pair   22 - SPP not init         23 - SPP has been init
 * 24 - Invalid inq mode     25 - Too long inq time   26 - No BT addresse       27 - Invalid safe mode
 * 28 - Invalid encryptmode  30 - FAIL
 */
int Disk91_hc05::sendHC05AtCmd(char * atcmdstr, boolean imediate) {
  int recvd = 0;
  char _buffer[128];
  int _bufsize = sizeof(_buffer)/sizeof(char);
  int ret=-1;

   print_debug("Entering sendHC05AtCmd()");
   
   print_debug2(" * Send : ",atcmdstr);
   blueToothSerial.print(atcmdstr);
   blueToothSerial.print("\r\n"); 
   delay(200);
   while ( ( recvd = blueToothSerial.readBytes(_buffer,_bufsize) ) == 0 && (! imediate) ) delay(100);
   
#ifdef DEBUG
     if (recvd > 0 ) {
       int i = 0;
       while ( i < _bufsize-1 ) {
          if ( _buffer[i] == '\n' ) { _buffer[i] = 0 ; i = _bufsize; }
          else i++;
       }
       print_debug2(" * Received :",_buffer);
     }
#endif
   if (recvd >= 2) {
     if ( _buffer[0] == 'O' && _buffer[1] == 'K' ) ret=-1;     
     else if ( _buffer[0] == 'E' ) {
       // Get Error number if _buffer[8] = ')' then 1 digit error code, else 2 digit
       if ( recvd >= 9 ) {
         ret = ( _buffer[8] != ')' )?16*(hex2dec(_buffer[7]))+(hex2dec(_buffer[8])):hex2dec(_buffer[7]);
         print_debug2(" * Error code : ",ret);
       }
     } else if ( _buffer[0] == 'F' ) {
       print_debug(" * Error code : FAIL");
       ret=COD_FAIL;
     }
   }
   // Other cases : consider as valid command
   print_debug("Leaving sendHC05AtCmd()");
   return ret;  
}


/* --- Get RName - get remote device name (mostly for debugging purpose in my case
 * Print name on debugging flow
 * Remote address is transmitted with ':' separator, it is converted to ',' in the function
 * AT+RNAME?34C0,59,F191D5
 */
void Disk91_hc05::getHC05RName(char * raddr) {
#ifdef DEBUG
  int recvd = 0;
  char _buffer[128];
  int _bufsize = sizeof(_buffer)/sizeof(char);
  int ret=-1;

   print_debug("Entering getHC05RName()");
   
   print_debug2(" * Request For : ",raddr);
   
   blueToothSerial.print("AT+RNAME?");
   blueToothSerial.print(raddr);
   blueToothSerial.print("\r\n"); 
   delay(5000);
   recvd = blueToothSerial.readBytes(_buffer,_bufsize);
   if (recvd > 0 ) {
       int i = 0;
       while ( i < _bufsize-1 ) {
          if ( _buffer[i] == '\n' ) { _buffer[i] = 0 ; i = _bufsize; }
          else i++;
       }
       print_debug2(" * Received :",_buffer);
   }
   // Other cases : consider as valid command
   print_debug("Leaving getHC05RName()");
#endif
}


