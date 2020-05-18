/* Copyright (c) 2018-2020 dbradley. */

#ifndef ESP8266
#ifndef ESP32
#error "Only for ESP32/8266 module"
#endif
#endif

/**  License found in 'DevFsUploadCommon.h'.

  Another file should accompany this file so as to be valid.
     'DevFsUploadCommon.h'

  The license description is contained within 'DevFsUploadCommon.h'
  along with usage instructions.

  If this files is not accompanied with a 'DevFsUploadCommon.h' file
  and the license intact. This .cpp file is suspect.

*/
#include <DevFsuploadCommon.h>

#include <Arduino.h>
#include <FS.h>
#include <WiFiClient.h>

#include <base64.h>

// the server object to operate through

#ifdef ESP32
#include <ESP32WebServer.h>
// #include <SPIFFS.h>

ESP32WebServer* DevFsUploadESP::serverPabc;

const char* DevFsUploadESP::projEspTarget  = " on ESP32";
#else
#include <ESP8266WebServer.h>

ESP8266WebServer* DevFsUploadESP::serverPabc;

const char* DevFsUploadESP::projEspTarget  = " on ESP8266";

#endif

// selected-dir in the upload dir: field, used for sync of setting
// between the Browser-UI and the ESP
String DevFsUploadESP::selectedDir = "/";    // starts at the root

// SPIFFS/LittleFS file names may not exceed 32 chars [31 + \0] of a char string
File DevFsUploadESP::fsUploadFile;
boolean DevFsUploadESP::uploadStarted = false;
boolean DevFsUploadESP::uploadAction = false;

String DevFsUploadESP::upldFileList = "";
String DevFsUploadESP::errUpl = "";
String DevFsUploadESP::errUplAdd = "";

const char* DevFsUploadESP::uploadAccessDefault PROGMEM = "/upload";

String DevFsUploadESP::simpleErrorMsg = "";

/**
    ***** BELOW *****

    Code has been really compressed to save storage space.

    This makes it hard to read in this context, but saves 100's of bytes
    PROGMEM and variable space. My normal verbose coding style (documented code)
    would result in too much storage.

    NOTE: The array constructs store into PROGMEM, but do have some variable
          stack space impacts too.
*/

/**
  The DOCTYPE html tag to indicate HTML 5 format for all the following
  HTML code. (Also, style and script sections.)

  C/C++ coding with '\' at end of line does string concatenation at compile
  time.

  The data will be uploaded to a browser as a continuous line of text,
  thus doing debug at the browser will be very hard to do.  But optimization
  on the ESP takes precedence.

  Each script function is presented as a separate array string concatenation
  which allows viewing/editing in this cpp file a little easier.
  NB: extra whitespace is removed for ESP storage optimization.
*/

//
// This code is based on original from ESP8266 documentation and webpage
// "A Beginner's Guide to the ESP8266" Chapter 11 SPIFFS, Uploading files to SPIFFS
//
// the code has been modified to provide only upload file to
// do development of applications SPIFFS/LittleFS http files more rapidly

DevFsUploadESP::DevFsUploadESP() {
}

/**
  setup the server with the appropriate handle function and onFileUpload
  capability with the default
*/
#ifdef ESP32
void DevFsUploadESP::setupUpLoad(ESP32WebServer* serverP) {
#else
void DevFsUploadESP::setupUpLoad(ESP8266WebServer* serverP) {
#endif
  // default 'ipaddr'/upload
  DevFsUploadESP::setupUpLoad(serverP, uploadAccessDefault);
}


/**
  setup the server with the appropriate handle function and onFileUpload
  capability with the setting
*/
// although the above and below can be put together, separate is better code maintanance
#ifdef ESP32
void DevFsUploadESP::setupUpLoad(ESP32WebServer* serverP, const char* access) {
#else
void DevFsUploadESP::setupUpLoad(ESP8266WebServer* serverP, const char* access) {
#endif
  //  record the server object for use within this class
  serverPabc = serverP;

  // set handles for the various needs
  // 1) GET->initial load, 2a) POST->onFileUpLoad 2b) POST->List,..
  //
  //      2a implies POST action/form and once the upload is
  //      complete the regular POST action happens with no parameters
  //
  serverPabc->on("/ajaxESP", HTTP_POST, DevFsUploadESP::handlerAjax);

  serverPabc->on(access, HTTP_GET, DevFsUploadESP::handleUploadPage); // 1a
  serverPabc->onFileUpload(DevFsUploadESP::handleFileUpload);            // 2a
  serverPabc->on(access, HTTP_POST, DevFsUploadESP::handleOther);
}

/**
   Completion page presented after upload or abort conditions happen.
   Along with any file-list or error information.
*/
void DevFsUploadESP::handleUploadPage() {  
  // has dual purpose as 'initial page' and 'on upload completed'
  // for an upload selection

  // all self contained (making the "strings" const char* PROGMEM increases
  // the program store significantly vs the code below)
  //
  // - on initial page, only the form is presented
  // - after an upload is completed or abort, upldFileList information is
  //   added/injected into HTML after the <form>
  // -- the HTTP response provides a completed page with upldFileList info
  //
  WiFiClient client = serverPabc->client();

  // do we have a simple error condition to report
  if (simpleErrorMsg != "") {
    serverPabc->send(200, "text/plain", simpleErrorMsg);
    simpleErrorMsg = "";

  } else {
    mainPage(client);

    if (uploadAction) {
      uploadStarted = false;
      uploadAction = false;
    }
  }
  client.flush(); client.stop();
}

void DevFsUploadESP::mainPage(WiFiClient client) {
  // the main page is a static page in a const char* xxxxx[]. Need
  // to calculate the size that is going to be used as the content-length
  // response in for the header
  //

  // although ESP32/ESP8266-WebServer libraries have functions to
  // send and the such, the lbraries are very simple and as such
  // lack more capabilities on them

  // zip format to upload

  // respondHttp200(client, GZIP, 1000, false);

  respondHttp200(client, GZIP, htmlByteLength, false);
  // EPS32 and ESP8266 compatible
  client.write_P(((PGM_P)&htmlByte[0]), htmlByteLength);
  client.flush(); client.stop();

  // due to the difference in PROGMEM mapping
  // the following works on ESP32, but not on EPS8266
  //
  // 32 works: client.write(&htmlByte[0], htmlByteLength);
  // 32 works: client.write(htmlByte, htmlByteLength);
}

/**
     Constructs the 200 response and supporting header information to
     be sent.
     client     is the client object to communicate with
     htmlText   true - will be text/html, false - text/plain content-type
     length     length in bytes of content that will be provided
     connectClose  default true: add to header the Connection: close
*/
// enum httpContentType{HTML, PLAIN, GZIP};

void DevFsUploadESP::respondHttp200(WiFiClient client, httpContentType type, int length, boolean connectionClose = false) {

  // although ESP32/ESP8266-WebServer library's have functions to
  // send and such, lack more involved capabilities to do responses
  // that are dynamic
  client.println( F("HTTP/1.1 200 OK"));

  // HTML and GZIP will be text/html
  client.print( F("Content-Type: text/"));
  client.println( type == PLAIN ? "plain" : "html");

  // GZIP needs to indicate the encoding
  if (type == GZIP) {
    client.println(F("Content-Encoding: gzip") );
  }
  if (length > -1) {
    client.print( F("Content-Length: "));
    client.println(length);
  }
  if (connectionClose) {
    client.println( F("Connection: Close") );
  }
  // close of the header
  client.println("");
}

/**
   Completion page presented after upload or abort conditions happen.
   Along with any file-list or error information.
*/
void DevFsUploadESP::sendComplete() {
  DevFsUploadESP::handleUploadPage();
}

/**
  handler for processing list, delete or view,.... requests
*/
void DevFsUploadESP::handleOther() {
  uploadAction = (serverPabc->args() == 0);

  if (uploadAction) {
    // assume on-file-upload has completed its request and
    // a regular POST happens to close off the upload action
    // which has 0 (zero) arguments
    upldFileList += "</table>";

  } else {

    // only download remains

    // process file requests if the "fn" key/arg is present
    // process directory requests if the "dn" key/arg is present
    //
    if (serverPabc->hasArg("fn")) {
      if (serverPabc->hasArg("down")) {
        // download is a self contained protocol, so this will return as the
        // page requestinng the download is unaffected.
        if (processDownload(serverPabc->arg("fn"))) return;

        simpleErrorMsg = F("Download failure");
      }
    }
  } // end uploadAction
  DevFsUploadESP::sendComplete();
}

/**
  process the file and return the content as text for viewing
*/
void DevFsUploadESP::processViewFile(String filePath) {
  //
  // buffers for processing the file for speedy transfer.
  //
  // ESP8266 and ESP32 buffer sizes are different between the two devices
  // due to memory architecture/management differences. Also, empirical 
  // application of buffer sizes has minimized the values too thise below.
  //
  // The buffer read or writes would crash, very notably on ESP8266 due to
  // smaller storage and 32-bit boundary  
  //
#ifdef ESP8266
// this appears to be a limit for ESP8266 (does not crash)
#define BUF_READ_FILE_SIZE 512
#define BUF_WRITE_VIEW_SIZE 1024
  
#else
    
#define BUF_READ_FILE_SIZE 1024
#define BUF_WRITE_VIEW_SIZE 2048   
  
#endif
    
  // buffers used for transferring data from a file to the client (as
  // hex encoded contented)
 
  // transfer as HEX-encoded conversion array
  const char* hexArr = "0123456789ABCDEF";
    
  char bufReadFile[BUF_READ_FILE_SIZE];
  char bufWriteVu[BUF_WRITE_VIEW_SIZE];
  
  // prepare the file for processing
  WiFiClient client = serverPabc->client();
  // open the file if possible
  File f = openFile4ReadTargeted(filePath);

  if (!f) {
    // not much can be done
    respondHttp200(client, HTML, 0);
  }else{ 
    // the view file is read as bytes and then converted to HEX char-pairs
    // so it may be transferred to the client as binary. Not needed for 
    // text files, but for image files the HEX encoding is needed.
    //    
    // consists of 3 parts where the length of all 3 determines
    // the content-length value for the http header information
  
    // an image file is optional and secial HTML for image display and
    // processing is required (the following images are supported)
    boolean imageFileBool = filePath.endsWith(".png") 
          || filePath.endsWith(".jpg")
          || filePath.endsWith(".bmp") 
          || filePath.endsWith(".gif");
          
    // calculating the size for output is most important for speed
    // between the this-server and client
    
    // part1 (static) -- is the view file information and textarea container for 
    // text/image as characters
    String part1 = F("view=<p>Viewing: ");
    part1 += filePath;
    part1 += F("</p><textarea id='vutxt' style='width:95%;' spellcheck='false' rows='15'>");
    
    // part2 (dynamic) -- conversion of file contents to encoded-HEX for transmission of
    // binary content
    
    // part3 (static) -- close of the textarea and optional image display field
    String part3 = "</textarea>";
    
    // image is optional
    if (imageFileBool) {
      String imgType = filePath.substring(filePath.lastIndexOf(".") + 1);
      
      // if the file is an image then process into an img tag
      part3 += "<div id='imgoffile'>";
      part3 += imgType;
      part3 += "</div>";
    }
    // sum the lengths of all parts
    int lengthAll = part1.length() + 2;  // +2 for println
    lengthAll += f.size() * 2;   // dynamic but is file char --> HEX encoded (* 2)
    lengthAll += part3.length() + 2;
    
    // send the header with the expected length to transmit and part1
    respondHttp200(client, HTML, lengthAll);
    client.println(part1);
        
    // part2 -- convert file chars/bytes to HEX-encoded chars for binary transfer
    // without using WebServer tools
    int bufWriteVuPtr = 0;
    int lenRead = 0; 
 
    // process the file in-blocks
    while (f.available()) {
      lenRead = f.read((uint8_t*)bufReadFile, BUF_READ_FILE_SIZE);
      
      // HEX-encode the read-chars
      for(int n = 0; n < lenRead; n++){
         char c = bufReadFile[n];          
   
         bufWriteVu[bufWriteVuPtr] = hexArr[(c >> 4) & 0x0f];
         bufWriteVuPtr++;
         
         bufWriteVu[bufWriteVuPtr] = hexArr[c & 0x0f];
         bufWriteVuPtr++;
      }
      // write the HEX buffer to the client
      client.write((uint8_t*)bufWriteVu, bufWriteVuPtr);
      bufWriteVuPtr = 0;
      
      // consider a yield at some point in time (if not ESPAsyncWebServer)
    }
    f.close();
    
    // part3 -- end of textarea and the optional image display HTML
    client.println(part3);
  }
  client.flush();
  client.stop();
}

/**
  process the file and download it to the browser-client for saving

  return true is success, false for failure
*/
boolean DevFsUploadESP::processDownload(String fn) {
  // SPIFFS and LittleFS are slightly different for open
  File f = openFile4ReadTargeted(fn);

  int idx = fn.lastIndexOf("/");
  if (!f || idx == -1) {
    return false;
  }
  // may download the file (to browser), but provide filename without path
  serverPabc->sendHeader( F("Content-Type"), F("text/text") );

  String fnNoPath = fn.substring(idx + 1);
  serverPabc->sendHeader( F("Content-Disposition"), "attachment; filename=" + fnNoPath);

  serverPabc->streamFile(f, F("application/octet-stream") );
  f.close();

  return true;
}

/**
  handlerAjax call for the directory selected request

  Note:
  ESP8266WebServer and ESP32WebServer work differently when sending to
  the client in response. ESP32 on client.stop() does not close the connection
  and as such the Content-Length is required in the commuication.
  OTHERWISE, - the browser timeout on responses recieved comes into play. This
  timeout is tyically 2 seconds delay to the renderering of the
  browser display.
  - For the below ajax responses, 2 seconds for browser reaction
  is too long, so code reflects Content-Length capability.

*/

#define CONTENT_HOLDER_SIZE 100
// #define PTR_CHAR 1
// #define PTR_STRING 2

void DevFsUploadESP::handlerAjax() {
  WiFiClient client = serverPabc->client();

  // generic array of pointers that will hold a const char* or
  // String*
  const char* arrHolder[CONTENT_HOLDER_SIZE];

  // // arrOfType will be an int value that reresents different
  // // types of pointer kinds (0, not assigned
  // // 1 - const char*`
  // // 2 - String*
  // //
  // int arrOfType[CONTENT_HOLDER_SIZE];

  int iOfArrs = 0;

  // assume list-files and list-dirs are assumed to not be refreshed
  // HOWEVER, if the list-file or list-dirs action (eg. Delete file)
  // affects the contents of the list-xxxx, then set for refresh in the
  // conditional code below
  boolean listFilesAffected = false;
  boolean listDirsAffected = false;

  boolean preProcessed = false;

  String content = "";
  // String errContent = "";

  // the action needs to be processed, for the most partial_sort
  // there is no precedence for the action requests
  String action = serverPabc->arg("act");

  //d Serial.print("action: "); Serial.println(action);

  if (action == "onload") {
    preProcessed = true; //?
    // this is a request for information that is strict format
    content = "onload=";
    content += String(projTitleTarget);
    content += String(projEspTarget) + ",";
    content += String(selectedDir) + ",";
    content += (upldFileList == ""
                && errUpl == "") ? "noupld," : "yesupld,";

    arrHolder[iOfArrs] = content.c_str();
    iOfArrs++;

  } else if (action == "upldlst") {
    preProcessed = true;
    // request for known upload-list content is being asked for
    // const char* prefix = "upldlist=<br>Last upload: ";
    arrHolder[iOfArrs] = "upldlist=<br>Last upload: ";
    iOfArrs++;

    content = upldFileList;

    arrHolder[iOfArrs] = content.c_str();
    iOfArrs++;

    // calculate the length of the message to be sent
    // int contentLen = strlen(prefix);
    // contentLen += upldFileList.length();
    if (errUpl != "") {
      content += F("<div id='errupld'>");
      content += errUpl;
      content += errUplAdd;
      content += "</div>";

      arrHolder[iOfArrs] = content.c_str();
      iOfArrs++;
    }
    // reported once only
    errUpl = "";
    errUplAdd = "";
    upldFileList = "";

  } else if (action == "seldir") {
    preProcessed = true;
    // set the selected directory to be the prefix for
    // files being uploaded (HTML file-upload does not
    // provide the hosts browsers directory paths, only
    // file names)
    selectedDir = serverPabc->arg("dn");

    arrHolder[iOfArrs] = "okay";
    iOfArrs++;

  } else if (action == "delete") {
    deleteFileTargeted(serverPabc->arg("fn"));
    listFilesAffected = true;

  } else if (action == "view") {
    // respondHttp200(client, HTML, -1);
    processViewFile(serverPabc->arg("fn"));
    // client.flush(); client.stop();
    return;

    // "download" is excluded from ajax as its a "separate" ajax type
    // of connection via a HTML-HTTP protocol

  } else if (action == "mksubdir") {
    listDirsAffected = true;

    if (serverPabc->hasArg("dnsub")) {
      if (serverPabc->arg("dnsub") != "") {
        // make the directory with the full-paths
        mkDirFSTargeted(serverPabc->arg("dn") + serverPabc->arg("dnsub"));
      }
    }

  } else if (action == "rmdir") {
    listDirsAffected = true;

    // remove the directory, but remove all internal files before hand
    // (NB: remove directory when the last file in its
    // structure is removed too)
    DevFsUploadESP::rmDirFSTargeted(serverPabc->arg("dn"));

  } else if (action == "list") {
    // list request
    if (serverPabc->hasArg("files")) {
      listFilesAffected = true;

    } else if (serverPabc->hasArg("dirs")) {
      listDirsAffected = true;
      listDirsAffected = true;
    }
  }
  // list either files or dir(s) as appropriate (they are mutually exclusive)
  if (listFilesAffected || listDirsAffected) {
    // the key for the ajax response

    content = listFilesAffected ? "lfiles" : "ldirs";
    content += F("=<div class='scrl' tabindex='-1'><div class='bof dtb' id='");

    // istFilesAffected ? does not support the F(... usage so long coded,
    // but does not take any additional memory
    // content += listFilesAffected ? "lfiles'>" : "ldirs'><div>/ 0</div>";
    if (listFilesAffected) {
      content += "lfiles' > ";
    } else {
      content += F("ldirs'><div>/ 0</div>");
    }
    // put in place any other found directories or files
    content += DevFsUploadESP::listFilesOrDirsFSTargeted("/", listFilesAffected);

    arrHolder[iOfArrs] = content.c_str();
    iOfArrs++;

    arrHolder[iOfArrs] = "</div></div>";  // may not be F(.....);
    iOfArrs++;
  }
  respondOutput(client, iOfArrs, arrHolder);
}


void DevFsUploadESP::respondOutput(WiFiClient client, int iOfArrs,  const char* arrHolder[]) {

  int totalLen = 0;
  // get the total length of the output
  for (int i = 0; i < iOfArrs; i++) {
    totalLen += strlen(arrHolder[i]);
    // each is printed on a new line
    totalLen += 2;
  }
  respondHttp200(client, PLAIN, totalLen);
  // now do the output to the client
  for (int i = 0; i < iOfArrs; i++) {

    //d Serial.println(arrHolder[i]);
    client.println(arrHolder[i]);
  }
  client.flush(); client.stop();
}

/**
  upload a new file to the SPIFFS protocol handler
*/
void DevFsUploadESP::handleFileUpload() {
  HTTPUpload& upload = serverPabc->upload();
  uploadAction = true;

  // the HTTP upload protocol operates as a "state-machine"
  // 1) UPLOAD_FILE_START: requires the file to be opened for writing
  // 2) UPLOAD_FILE_WRITE: a block of data to write
  // 3) UPLOAD_FILE_END: close of the file
  // 4) UPLOAD_FILE_ABORTED: upload is aborted
  //
  // The code below is optimized to check for UPLOAD_FILE_WRITE 1st
  // as the most often task
  //
  // file is open and should be processing
  // ASSUMPTION: protocol will work correctly
  //
  if (uploadStarted) {
    if (upload.status == UPLOAD_FILE_WRITE) {
      if (errUpl == "") {
        fsUploadFile.write(upload.buf, upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (errUpl != "") {
        uploadStarted = true;
        serverPabc->send(200, F("text/plain"), "done");

      } else {
        // If the file was successfully created
        fsUploadFile.close();
        uploadStarted = false;

        // OPTIMIZATION  from -> upldFileList += "<td>" + String(upload.totalSize) + "</td></tr>";
        upldFileList += F("<td>");
        upldFileList += String(upload.totalSize);
        upldFileList += F("</td></tr>");
      }

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      Serial.print("Aborted: ");
      Serial.println(upload.filename);
    }
  } else {
    // file not open so expect an upload protocol error

    if (upload.status != UPLOAD_FILE_START) {
      if (errUpl == "") {
        errUpl = F("500: protocol order issue");
      }
    } else {
      // dirSelected variable needs to prefix the file name
      // and it is assumed to be ' / xxxxx / ' in format (as per the protocol)
      String fn = selectedDir;
      fn += upload.filename;

      if (upldFileList == "") {
        upldFileList = F("<table class='tab'><tr><th>File name</th><th>Size</th></tr>");
      }
      // OPTIMIZATION from -> upldFileList += "<tr><td>" + fn + "</td>";
      upldFileList += F("<tr><td>");
      upldFileList += fn;
      upldFileList += F("</td>");

      // Open the file for writing in SPIFFS/LittleFS and overwrite
      // (create if it doesn't exist)
      //
      // leave open as its closed on the "state-machine" UPLOAD_FILE_END state
      fsUploadFile = DevFsUploadESP::openFile4WriteTargeted(fn);

      if (!fsUploadFile) {
        errUpl = F("500: fail create file: ");
        errUplAdd = String(fn);
      } else {
        uploadStarted = true;
        errUpl = "";
      }
    }
  }
}

// The SPIFFS/LittleFS implementation for the ESP8266 supports filenames of up to 31 characters + terminating zero (i.e. char filename[32]), and as many subdirectories as space permits.
//
// Filenames are assumed to be in the root directory if no initial “/” is present.
//
// Opening files in subdirectories requires specifying the complete path to the file (i.e. open("/sub/dir/file.txt");). Subdirectories are automatically created when you attempt to create a file in a subdirectory, and when the last file in a subdirectory is removed the subdirectory itself is automatically deleted. This is because there was no mkdir() method in the existing SPIFFS filesystem.
//
// Unlike LittleFS unlike SPIFFS, the actual file descriptors are allocated as requested by the application, so in low memory conditions you may not be able to open new files. Conversely, this also means that only file descriptors used will actually take space on the heap.
//
// Because there are directories, the openDir method behaves differently than SPIFFS. Whereas SPIFFS will return files in “subdirectories” when you traverse a Dir::next() (because they really aren’t subdirs but simply files with “/”s in their names), SPIFFS will only return files in the specific subdirectory. This mimics the POSIX behavior for directory traversal most C programmers are used to.


