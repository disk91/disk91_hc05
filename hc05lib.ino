/*
THE disk91_HC05 SOFTWARE IS PROVIDED TO YOU "AS IS," AND WE MAKE NO EXPRESS 
OR IMPLIED WARRANTIES WHATSOEVER WITH RESPECT TO ITS FUNCTIONALITY, OPERABILITY,
OR USE, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES OF 
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR INFRINGEMENT. 
WE EXPRESSLY DISCLAIM ANY LIABILITY WHATSOEVER FOR ANY DIRECT, INDIRECT,
CONSEQUENTIAL, INCIDENTAL OR SPECIAL DAMAGES, INCLUDING, WITHOUT LIMITATION,
LOST REVENUES, LOST PROFITS, LOSSES RESULTING FROM BUSINESS INTERRUPTION OR 
LOSS OF DATA, REGARDLESS OF THE FORM OF ACTION OR LEGAL THEORY UNDER WHICH 
THE LIABILITY MAY BE ASSERTED, EVEN IF ADVISED OF THE POSSIBILITY OR LIKELIHOOD 
OF SUCH DAMAGES.
*/

#include <disk91_hc05.h>

Disk91_hc05 hc05;

void setup()
{
 #ifdef DEBUG
  pinMode(13,OUTPUT); // Use onboard LED if required.
  Serial.begin(9600); // Allow Serial communication via USB cable to computer (if required)
  while (!Serial) ; // whait for Serial to be open on desktop (re-open in case of reset)
                    // Seems that after loading code, this pause it not taken into account...
  print_debug("Entering setup()");

  for ( int i = 0 ; i < 2 ; i++ ) {
   digitalWrite(13,LOW); //Turn off the onboard Arduino LED
   delay(1000);
   digitalWrite(13,HIGH); //Turn off the onboard Arduino LED   
   delay(1000);
  }
  digitalWrite(13,LOW); //Turn off the onboard Arduino LED
#endif

  
 hc05.setupBlueToothConnection("test");
    
}

void loop()
{
  char buf[50];
  int bufd;
  
  while (1) {
     // Blocking operation
     if ( hc05.connect() ) {
       
       while ( (bufd = hc05.receive(buf,50)) >= 0 ) {
           buf[bufd]=0;
           Serial.print(buf);  
           
           if (Serial.available()){
              bufd = Serial.readBytes(buf,50);
              buf[bufd]=0; 
              hc05.send(buf);
           }
           
       }
     } 
    
  }
}

