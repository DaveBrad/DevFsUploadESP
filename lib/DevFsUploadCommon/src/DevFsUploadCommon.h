/* Copyright (c) 2018-2020 dbradley. */

#ifndef DevFSUploadCommon_h
#define DevFSUploadCommon_h

#ifndef ESP8266
#ifndef ESP32
#error "Only for ESP32/8266 module"
#endif
#endif

// Below contains 3 sections:
// 1) The license for this license
// 2) how to use the library
// 3) Code for the DevFsUpload common class

/**
License: 31st March 2020.

  This license applies to the 'DevFsUploadESP' group of library program(s)
  for use on/with Arduino IDE.
  
 Definition
  A "library" is comprised of a set of files, as listed below, which 
  are grouped together to be an executable library-program. 
     (C/C++ files .h and .cpp pairs are implied [.h/.cpp])
   
     - DevFsUpload.h/.cpp
     -                    DevFsUploadCommon.h/.cpp
                                   |
                  +----------------.---------------+
                  |                                |
         DevFsUploadSPIFFS.h/.cpp         DevFsUploadLittleFS.h/.cpp
         
                         [one or the other]
     
   "personal use" is a person using the library(ies) for there own 
   development.
   
   "entity use" is using for development purposes within a collaborative
   project, comprising persons or teams within an organization.

  Documentation is provided with the program in its Github repository 
  and is also covered by this license.
  
 Conditions
  1. Use is royality free for development activities (testing, 
     design,....) both commerical and non-commerical.

  2. Modification may be made with the provision:
     a) That a different class name is used. That is; not 
        'DevFsUploadSPIFFS' or 'DevFsUploadLittleFS' or 'DevFsUploadCommon' 
        or 'DevFsUploadESP'.
     b) An attribute to original library-author(s) is provided in the 
        new code/class.
     c) An "origin" statement/pointer to original library source is 
        provided in the new code/class.
     d) Sub-licensing to a more open/permissive/closed license is 
        not permitted.

  3. The library is provided AS-IS and has no warranty as to fitness 
     for use. Determining fitness for use is the responsibility of 
     the user.

  4. This library may not be productized, that is provided 
     with/along/packaged-with a consumer product, either commerical or 
     non-commerical.

  5. The user accepts the limitations below as necessary for the 
     program to be functional and none interferring with a users
     product/project development.

  6. The "library" is not open-source.

  7. Modification allows other persons/entities to re-use the 
     intelluctual property for 'personal use' or for 'entity
     use', but not change these license conditions.

  Limitations:
  As a program/library there are working limits on an embedded MCU
  device that the "library" does not cater for. Unknown/unpredictable
  failures will occur if the user pushes the MCU device beyond its
  capabilities using the "library".

  The program is meant as a development tool/aid, not a product, and as
  such has limited fault tolerence code.

  The program interacts with other sub-systems that it has no control
  over: HTML, Javascript, Javascript access to client PC file
  structures,......... which restrict capability.
  
  License-end
  
 Why this license. 
  All the GNU, GPL, MIT,.... typical licenses allow modifications where
  same-name may be used and deployed, causing confusion for users and
  causes trust issues. Additionally, 
  - typical license(s) allow productization, this library would not 
    be suitable as a product.   
*/

/**
   --------------- how to use the library -----------------
 Purpose:  upload and/or manage files.directorie on an ESP32/8266 to allow rapid
           development using SPIFFS/LittleFS (collectively refered to 'FS')
		   
Restriction: If the user's program does file-upload via the server handler, then this
             library is not compatible.

WARNING: code is not re-entrant and only supports ONE browser request at a time
         for uploading file(s) or managing the 'FS'

Purpose:
  During development on a 'FS' supported device (eg. ESP8266,...) using an
  IDE to upload files into the 'FS' is often time consuming.

  This method allows files to be uploaded to the 'FS' device independently
  of the IDE via a browser interface.
  
  The library is optionally installed, as outlined in below code example.

Limitations:
  1-20 file(s) upload at a time, greater than 20 is possible but RAM memory
  will be used while processing the file-uploaded-info and if too many, may cause 
  a crash.

Browser access:
http://<host-of-device>/upload      eg. http://192.168..0.100/upload


Usage in code:
The user needs to choose either SPIFFS or LittleFS as the file system on
their device. 
  #include <DevFsUploadSPIFFS.h>     or     #include <DevFsUploadLittleFS.h>
   
NB: example code below DevFsUpload'FS' implies using 'SPIFFS'/'LittleFS' in 
place of 'FS' [no quotes]

-- To use edit a project's .ino file after include section, 
-- include the DevFsUploadSPIFFS.h or DevFsUploadLittleFS, else do not 
-- in the setup(after server is defined) place the "DevFsUploadInstall" appropriately.
 
--------- '+' in the left column indicates what needs to be inserted so as to use 
--------- comment #include line to disable the installation and use of the library

 Example 1                                    Example 2
 ------------------------------------         -------------------------------------
 Server is defined as a pointer               Server is typically shown in examples
 which will be 'set' in the setup             as being just after the include headers
 function.                                    section.

                                              Note: &server
 ------------------------------------         -------------------------------------
 |  ::                                        |  ::
+|  #include <DevFsUpload'FS'.h>             +|   #include <DevFsUpload'FS'.h>
 |  ..                                        |   ..   
 |  ESP8266WebServer* server;                 |   ESP32WebServer server(80);
 |  ..                                        |   ..
 |  setup(){                                  |  
 |   ..                                       |
 |   server = new ESP8266WebServer(80);       |   setup(){
 |   ..                                       |    ..
+|   // install DevFsUpload server handle    +|    // install DevFsUpload server handle
+| #ifdef DevFsUploadInstall                 +|   #ifdef DevFsUploadInstall
+|     DevFsUploadInstall(server);           +|      DevFsUploadInstall(&server);    
+| #endif                                    +|   #endif
 |    ..                                      |    ..
 
                    Example 3 (disable DevFsUpload functionality from code)
                    ------------------------------------  
                    Do not include DevFsUpload'FS'.h in product deployed versions
	                OR better yet, remove the DevFsUpload lines completely
						
			    	NB: comment out ->  '// #include DevFsUpload'FS'.h'
                    ------------------------------------ 
                     |  ::
                    +|  // #include <DevFsUpload'FS'.h>
                     |  ..
                     |  ESP8266WebServer* server;
                     |  ..
                     |  setup(){ 
                     |   ..
                     |   server = new ESP8266WebServer(80);
                     |   ..
                     |   // install DevFsUpload server handle
                     | #ifdef DevFsUploadInstall
                     |     DevFsUploadInstall(server);
                     | #endif 
                     |    ..


                      ------------------------------------
      DevFsUploadInstall installs the following code equivalent
      - - - - - - - - - 
      --- '<host>/upload'      GET/POST for upload action
      --- 
      ------ server->on("/upload", HTTP_GET, DevFsUploadESP::handleUploadPage);
      ------ server->onFileUpload(DevFsUploadESP::handleFileUpload);
      ------ server->on("/upload", HTTP_POST, DevFsUploadESP::handleOther);
	  ------ server->on("/ajax", HTTP_POST, DevFsUploadESP::handleAjax);

 The user has nothing else to do.
*/


#define DevFsUploadInstall(...) DevFsUploadESP::setupUpLoad(__VA_ARGS__)

#include <Arduino.h>
#include <FS.h>

#ifdef ESP32
#include <ESP32WebServer.h>

#else
// LittleFS became a part of the ESP8266 core in 2019/2020, so available
// by default if a user updates their IDE.
#include <ESP8266WebServer.h>
#endif


class DevFsUploadESP {
  public:
    DevFsUploadESP();

    /**
      setup the server with the appropriate handle function and onFileUpload
      capability
    */
#ifdef ESP32
    static void setupUpLoad(ESP32WebServer* serverP);
    static void setupUpLoad(ESP32WebServer* serverP, const char* access);
#else
    static void setupUpLoad(ESP8266WebServer* serverP);
    static void setupUpLoad(ESP8266WebServer* serverP, const char* access);
#endif

    // ------ PRIVATE ------
  private:

    // ESP32 and ESP8266 are at different but similar

#ifdef ESP32
    static ESP32WebServer* serverPabc;
#else
    static ESP8266WebServer* serverPabc;
#endif

    /**
      Target methods that reflect the differences between SPIFFS and LittleFS
      processing needs. Also, typically, within these methods special #ifdef ESP32
      preprocessing code may be necessary.

      An example is:
      - SPIFFS 8266 openDir gives Dir type,
       - whereas SPIFFS 32 givesFile type
      as their return. DevFsUpload extracts what datum it requires
      and then continues with 'common' code/processing.
    */
    static void listFilesOrDirsFSTargeted (String dirname, WiFiClient client, boolean listFiles, boolean startingAtRoot);
    static void rmDirFSTargeted(String dirname);
    static void mkDirFSTargeted(String nuDirFullPath);
    static File openFile4WriteTargeted(String fn);
	static File openFile4ReadTargeted(String fn);
	static void deleteFileTargeted(String fn);

    static const char* projTitleTarget;
    
    // string to use for access to the upload webpage (default: "/upload")
    static const char* uploadAccessDefault;

    // indicators for upload processing and what is being uploaded
    // to the ESPxxx device from the browser (upload of only one file
    // file at a time is supported)
    static boolean uploadStarted;
    static boolean uploadAction;
    static File fsUploadFile;

    // indicators that cause updates to the list files/directory zones
    // of the upload page (when requests have been processed do a list-files
    // or list-dirs action due to changes in file/dir content)
    static boolean listFilesAction;
    static boolean listDirsAction;

    // list of files that were uploaded in a HTML format for display at the browser
    // error fields when upload errors are detected and need to be reported to
    // the HTML browser
    static String upldFileList;
    static char* errUpl;
    static String errUplAdd;

    // the file name og the a request to view the contents of a file
    static String viewFileName;

    // the directory on the ESP that is selected (when a file upload request
    // is recieved only the file name is provided, this variable holds
    // the directory or sub-directory to store the file(s) too)
	static void setDirSelected(String setStr);

    // HTML code for the processing of files
    static const char* lastFileTable1;
    static const char* lastFileTable2;

    static const char* doctypeHTMLArr1[];
    static const char* doctypeHTMLArr2[];

    // the main page has three sections which require ESP server data
    //

    static String atatArr[];

    static const char* mainPgArr[];
    static const char* mainPgListZoneArr[];

    static String alterString(String strToAlter);

    // WebServer handler methods
    static void handleUploadPage(); // main page
    static void handleFileUpload(); // post upload
    static void handleAjax();       // post ajax requests

    static void handleOther(); // requests and the such

    // Webserver responses methods
    // -- for the most part requests are responded too by the internal
    // --- methods associated with the request, direct client.print/println
    // --- text
    static void sendComplete();
    static void respondHttp200(WiFiClient client, boolean htmlText);
    static void doctypeBody(WiFiClient client);

    static void clientPrtLn(WiFiClient client, int lenArr, const char* arr[]);

    // method to process the view-file request via HTTP
    static void processViewFile();
};
#endif
