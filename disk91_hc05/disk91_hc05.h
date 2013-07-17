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
#include "Arduino.h" 
#ifndef DISK91_HC05
#define DISK91_HC05

#define DEBUG

/* ======================================================================
 * Configuration
 * ======================================================================
 */

// -- Pairing timing
#ifndef MAX_SLAVE_TIME
#define MAX_SLAVE_TIME    60        // Wait as a slave for MAX_SLAVE_TIME seconds
#endif

#ifndef MAX_MASTER_TIME
#define MAX_MASTER_TIME   25        // Wait as a master for MAX_MASTER_TIME * 1.28s (max is 48)                                  
#endif

// -- Serial port used for hc05 communication
#ifndef blueToothSerial
#define blueToothSerial Serial1
#endif

 
/* ======================================================================
 * Helper and constants
 * ======================================================================
 */
#ifdef DEBUG
  #define print_debug(x) Serial.print("Debug :");Serial.println(x);
  #define print_debug2(x,y) Serial.print("Debug :");Serial.print(x);Serial.println(y);
#else 
  #define print_debug(x) 
  #define print_debug2(x,y) 
#endif
#define hex2dec(x) ((x>'9')?10+x-'A':x-'0')

// -- HC05 States
#define ST_INITIALIZED 0
#define ST_PAIRED 3
#define ST_DISCONNECTED 7
#define ST_PAIRABLE 2
#define ST_INQUIERING 4
#define ST_ERROR -1
#define ST_CONNECTED 6
#define ST_NOFORCE -2
#define ST_SEARCH_FOR_PAIR  -3

// -- Internal
#define COD_FAIL  30
#define BUFSZ 50

 
 class Disk91_hc05
 {
   public:

       /* -------------------------------------------------------------
        * Construction
        * -------------------------------------------------------------
        */
       Disk91_hc05();

       /* -------------------------------------------------------------
        * Configure the bluetooth device
        * --------------------------------------------------------------
        */
       boolean setupBlueToothConnection(char *);

       /* -------------------------------------------------------------
        * Establsh connection to a device. return only when the connection
        * has been done with true
        * --------------------------------------------------------------
        */
       boolean connect();
       
       /* ---------------------------------------------------------------
        * Receive string from bluetooth - not blocking operation
        * return number of char received
        * return -1 when the connection is broken
        * ---------------------------------------------------------------
        */
       int receive(char *,int);
       
       /* ---------------------------------------------------------------
        * Send string over bluetooth - blocking operation
        * return false if disconnected
        * ---------------------------------------------------------------
        */
       boolean send(char *);
   
   private:
      boolean getHC05Connection();  
      void forceHC05State(int);
      int getHC05State();
      int startHC05Inq(int);
      boolean getHC05Mrad();
      int getHC05ADCN();
      int sendHC05AtCmd(char *, boolean);
      void getHC05RName(char *);
        
      // list of devices detected
      char detected_address[4][16];
      int  detected_addressN;
      unsigned long rates[6];
      int forcedState;
      
      // state memory
      boolean bootup;                    // true a boot to validate / unvalidate pairing
      boolean reqPairing;                // switch to true to execute a pairing or re-pairing search
      boolean initSuccess;		 // true if init finished in success
   
 };
 
#endif
