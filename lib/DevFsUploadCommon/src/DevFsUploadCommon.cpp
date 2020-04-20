#include <Ethernet.h>

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

// the server object to operate through

#ifdef ESP32
#include <ESP32WebServer.h>
// #include <SPIFFS.h>

ESP32WebServer* DevFsUploadESP::serverPabc;
#else
#include <ESP8266WebServer.h>

ESP8266WebServer* DevFsUploadESP::serverPabc;
#endif

boolean DevFsUploadESP::uploadStarted = false;
boolean DevFsUploadESP::uploadAction = false;
boolean DevFsUploadESP::listFilesAction = false;
boolean DevFsUploadESP::listDirsAction = false;

// SPIFFS file names may not exceed 32 chars [31 + \0] of a char string
String DevFsUploadESP::viewFileName;
File DevFsUploadESP::fsUploadFile;

String DevFsUploadESP::upldFileList = "";
char* DevFsUploadESP::errUpl = "";
String DevFsUploadESP::errUplAdd = "";

const char* DevFsUploadESP::lastFileTable1 = "<style>.tab td:nth-child(2){text-align: right;}</style>";

const char* DevFsUploadESP::lastFileTable2 = "<table class='tab'><tr><th>File name</th><th>Size</th></tr>";

const char* DevFsUploadESP::uploadAccessDefault = "/upload";


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
const char* DevFsUploadESP::doctypeHTMLArr1[] = {
    /* Arduino IDE places this into PROGMEM without the keyword */

  "<!DOCTYPE HTML><html lang='en'><head><meta charset='UTF-8'>",

  /**
    NB: coding style using '\' optimizes both ESP program storage and global
        variable use. Retain the style and check if making changes for increased
        storage usage.

        ->  use color code to save 2 bytes vs name
        orangered  #ff4500
        lightblue  #8fd8d8
  */
  "<style>\
.btt{border-radius:15px;}\
.btt:focus{border:2px solid blue;}\
.btt2{color:red;}\
.btt3{width:0px;height:18px; padding-right:15px;background:#f2fcf9;}\
.info{font-style:italic;font-size:0.8em;}\
.bof{direction:ltr;float:left;}\
.scrl{margin-left:24px;height:155px;overflow-y:scroll;direction:rtl;}\
.dvtb{display:table;}\
.frmRow{display:table-row;}\
.frmRow div:nth-child(1) input{border:0px;}\
.tdi{display:table-cell; padding-right: 10px;}\
.siz{text-align:right;}\
.prjnam{color:#ff4500;text-decoration:underline;padding-bottom:10px;}\
.dvtb form:nth-child(3n) div:nth-child(1) input:nth-child(1),\
.dvtb form:nth-child(3n) div:nth-child(n-2){background:#ffffcc;}\
</style>"
};

const char* DevFsUploadESP::doctypeHTMLArr2[] = {
    /**
      global variables that are used in building script content dynamically
      via the @n@ architecture
    */
  "@<script>var g_ajax=\"@3@\";\
var g_psudo=\"style='font-style:italic;background:#8fd8d8;'\" ;",
  
   /** function gE

    Get element 'by id' as a function saves considerable bytes of
    script text.
  */
  "function gE(eId){return document.getElementById(eId);}",

  /**
     function that sets the upload button enabled when files or dir are browse
    selected AND the selected-dir (dir: field) is also valid.

    NB: coding style using '\' optimizes both ESP program storage and global
       variable use. Retain the style and check if making changes for increased
     storage usage.

    Code has been really compressed to save storage space.
    
    s    value of dirroot
    isD  isDirValid
    
    ufE  up-file-element    ufB up-file-button state   ufI up-file-input state
    udE  up-dir-element     udB up-dir-button state    udI up-dir-input state
  */
  "function upldbts(){\
var s=gE('dirroot').value;\
var isD=s.endsWith(\"/\")&&s.startsWith(\"/\");\
\
var ufE=gE(\'upfileinp');\
var udE=gE('updirinp');\
\
var ufB=true,udB=true;\
var ufI=false,udI=false;\
\
if(ufE.value!==''&&isD){\
ufB=false;\
udI=true;\
}\
if(udE.value!==''&&isD){\
udB=false;\
ufI=true;\
}\
ufE.disabled=ufI;\
udE.disabled=udI;\
\
gE('upfilebtt').disabled=ufB;\
gE('updirbtt').disabled=udB;\
}",

  /**
     function that resets the browse-selected setting
     

    Code has been really compressed to save storage space.
  */
  "function resetF(formid){\
gE(formid).reset();\
upldbts();\
}",

  /**
    function to sort the file names for the 'list SPIFFS' request as
    SPIFFS gathers files in a none alphabetic order

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function sortTab(){\
sortList();\
sortDirs();\
}",

  /**
    Sorts the file-list table alphabetically for display.

    Also, inserts HTML code for controls based on raw data
    from the ESP-server for client-UI presentation.

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function sortList(){\
var tab=gE('lsttab');\
if(tab===null)return;\
var tLen=tab.childNodes.length;\
\
if(tLen===0)return;\
\
var arr=[];\
for(var i=0;i<tLen;i++){\
var a=tab.childNodes[i].innerHTML;\
if(a!==undefined)arr.push(a);\
\
}\
arr.sort();\
while(tab.firstChild)tab.removeChild(tab.firstChild);\
\
for(i=0;i<arr.length;i++){\
var str=arr[i];\
var idx=str.lastIndexOf(' ');\
var fn=str.substring(0, idx);\
var len=fn.length+2;\
var siz=str.substring(idx+1);\
\
var s=\"<form class='frmRow' method='post'>\
<div class='tdi'><input \";\
\
if(fn.endsWith(\"/\"))s+=g_psudo;\
\
s+=\"type='text' name='fn' readonly value='\"+fn+\"' size='\"+len+\"'></div>\
<div class='tdi siz'>\"+siz+\"</div>\
<div class='tdi'><input class='btt' type='submit' name='delete' value='Delete'></div>\
<div class='tdi'><input class='btt' type='submit' name='view' value='View'></div></form>\";\
tab.insertAdjacentHTML('beforeend', s);",
  "}\
}",

  // NB: significant extra ESP store is required if coding last lines as
  //      tab.insertAdjacentHTML('beforeend', s); }}", OR
  //      tab.insertAdjacentHTML('beforeend', s);\
  //      }\
  //      }",
  // vs.
  //  "tab.insertAdjacentHTML('beforeend', s);",
  //
  //  "}",
  //  "}",

  /**
     Sorts the directory-list table alphabetically for display.

    Also, inserts HTML code for controls based on raw data
    from the ESP-server for client-UI presentation.

    NB: coding style using '\' optimizes both ESP program storage and global
     variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function sortDirs(){\
var tab=gE('lstdirtab');\
if(tab===null)return;\
var tLen=tab.childNodes.length;\
if(tLen===0)return;\
var arr=[];\
for(var i=0;i<tLen;i++){\
var a=tab.childNodes[i].innerHTML;\
\
if(a!==undefined)arr.push(a);\
\
}\
arr.sort();\
while(tab.firstChild){tab.removeChild(tab.firstChild);}\
for(i=0;i<arr.length;i++){\
var str=arr[i];\
var idx=str.lastIndexOf(' ');\
var dn=str.substring(0, idx);\
var len=dn.length+2;\
var siz=str.substring(idx+1);",

  // select
  //
  // NB: coding style using '\' optimizes both ESP program storage and global
  //     variable use. Retain the style and check if making changes for increased
  //  storage usage.
  "var s=\"<div class='frmRow'><div class='tdi'><input \";",
  "if(siz===\"1\")s+=g_psudo;",

  "s+=\" type='text' readonly value='\"+dn+\"'\\\">\
<button class='btt' name='selectdir' onclick=\\\"setDf('\"+dn+\"');\\\">Select</button>\
</div>\";",

  // mk-subdir
  //
  // NB: coding style using '\' optimizes both ESP program storage and global
  //     variable use. Retain the style and check if making changes for increased
  //  storage usage.
  "s+=\"<div class='tdi'><form  method='post'>\
<input type='hidden' name='dn' readonly value='\"+dn+\"'>\
<input type='text' autocomplete='off' name='dnsub' value='' tabindex='0' oninput='mkval(this);'>\
<input class='btt' type='submit' disabled name='mksubdir' value='mk-subDir'>\
</form></div>\";",

  // rmdir
  //
  // NB: coding style using '\' optimizes both ESP program storage and global
  //     variable use. Retain the style and check if making changes for increased
  //  storage usage.
  "s+=\"<div class='tdi'><form id='rmdirform' method='post' onsubmit=\\\"return rmdirFn('\"+dn+\"');\\\">\
<input type='hidden' name='dn' readonly value='\"+dn+\"'>\
<input type='submit' class='btt btt2' name='rmdir' value='rmDir'>\
</form></div></div>\";",

  // insert the dir-line into the table
  "tab.insertAdjacentHTML('beforeend', s);",
  "}",
  "}",

  /**
    function for setDf (set dir field) which sets the select-dir field (dir:) and
    performs ajax action to the upload-server

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function setDf(dn){\
var dirroottxt=gE('dirroot');\
dirroottxt.value=dn;\
hndlAjax(dn);\
}",

  /**
    function for rmDir-function (rmdir is a key word)

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function rmdirFn(dn){\
var s=dn+\"\\n\\nrmDir not reverse-able\";\
return confirm(s);\
}",

  /**
    function for mkval validate the mk-suubdir input field.

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.
    
    ss - set state for disabled condition
    em - error message

    Code has been really compressed to save storage space.
    
    vE  ele.value
    ss  set-state to
    em  error-message
  */
  "function mkval(ele){\
var vE=ele.value;\
var ss=false;\
var em=\"\";\
\
if(vE.indexOf(\"/\")!==-1){\
em=\"'/'\";\
ss=true;\
\
}else if(vE.substring(0,1)===\" \"){\
ss=true;\
em=\"lead space\";\
}\
if(ss){alert(em+\"  not permitted\");}\
\
if(vE===\"\"){ss=true;}\
\
ele.nextElementSibling.disabled=ss;\
}",

  /**
    when the dir: changes

    NB: coding style using '\' optimizes both ESP program storage and global
     variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function oninpDir(){\
var upFld=gE('dirroot');\
upldbts();\
\
if(upFld.value.endsWith(\"/\"))hndlAjax(upFld.value);\
}",

  /**
     Function hndlAjax [handleAjax] will process changes to the slected-dir field and
    AJAX the info to the ESP server via XMLHttpRequest

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
    
    Do nothing on return from the server
    
    h http-object
  */
  "function hndlAjax(dn){\
var h=new XMLHttpRequest();\
var params='selectdir=do&fn='+dn;\
\
h.open('POST',g_ajax,true);\
h.setRequestHeader('Content-type','application/x-www-form-urlencoded');\
\
h.onreadystatechange=function(){};\
h.send(params);\
}",

   /* done so other scripts are added prior */
  "</script></head><body onload='sortTab();'>"

};

/**
   The atatArr is an array to hold strings that are different depending on
   the system being supported. The strings stored are dynamic in nature
   but may be set once or many times, depending on how the array elements are
   set-once or set many times.

   The define items below represent the element index of some data item.

   A set-once setting would be coded as:
      if(atatArr[DEFINE_NAME] == ""){
      // set-once
      atatArr[DEFINE_NAME="some string";
    }

   atatArr interfaces with clientPrtLn(.....) method and will in turn be processed
   for @n@ patterns.

   clientPrtLn accepts an array of string-elements, if a string-element starts with "@"
   it will cause the processing of an @n@ pair within it line.
   eg.
       "@<td colspan='3' class='prjnam'>DevFsUploadESP - @0@ on ESP@2@</td></tr>",

       -- "@0@" and "@2@" patterns will be replaced at run time.
     -- leading "@" is removed from the arrangement
*/

#define TITLE_FS_TYPE_IDX 0
#define DIR_SELECT_IDX 1
#define TITLE_ESP_NUMBER 2
#define AJAX_HANDLER_ADDR 3

String DevFsUploadESP::atatArr[] = {

  "",        // @0@ :: title for FS type 4 DevFsUploadESP:  'SPIFFS' or 'LittleFS' [manually set]
  "",        // @1@ :: dirSelect setting
#ifdef ESP32   // @2@ :: ESP number processing on. Set-once and static per say.
  "32",
#else
  "8266",
#endif
  "",    // @3@ :: the string for the ajax handler for upload eg. 'uploadajax'

  ""    // end of array, insert new items be this
};

/**
  mainPgxxxxxxxArr arrays contain the HTML code for the main-page of DevFsUploadESP.

  Certain elements have @n@ alteration 'fields' that will be dynamically altered
  during run-time to reflect current data.
*/
// browse buttons
const char* DevFsUploadESP::mainPgArr[] = {
  // title zone
  "<table>",

  // - browse button zone
  // -- directory selected info, input
  "<tr><td>1.</td>",
  "<td colspan='2' style='text-align:center'>",
  "upload dir: <input type='text' id='dirroot' name='dirroot' oninput='oninpDir();'",
  "@value='@1@'",
  "size='32'><span class='info'> Select dir from 'List Dir...'</span></td></tr>",

  // -- browse & upload for file and directory buttons
  // ---- upload file(s)

  "<tr><td>2.</td><td><form method='post' id='upfile' enctype='multipart/form-data'>",
  "<button class='btt btt3' type='button' onclick='resetF(\"upfile\");'>R</button>",
  "<input type='file' id='upfileinp' name='name' multiple onchange='upldbts();'>",
  "<br>",
  "<button class='btt' id='upfilebtt' disabled>Upload file(s)</button> to dir",
  "</form></td>",

  // ---- upload diectory
  "<td> <form method='post' id='updir' enctype='multipart/form-data'>",
  "<button class='btt btt3' type='button' onclick='resetF(\"updir\");'>R</button>",
  "<input type='file' id='updirinp' name='name' multiple webkitdirectory mozdirectory onchange='upldbts();' >",
  "<br>",
  "<button class='btt' id='updirbtt' disabled>Upload Directory</button> all files: browse-dir + sub-dir(s)"
  "</form></td></tr>",

  // --  information
  "<tr><td></td><td colspan='2' class='info'>1-20 files ideal to prevent memory issues in upload execution on device.</td></tr>",
  "</table>"
};

const char* DevFsUploadESP::mainPgListZoneArr[] = {
  // - list zone
  // -- list files & dir(s)
  "<table><tr> <td><form method='post'>",
  "<input class='btt' type='submit' name='list' value='List Files'></form></td>",
  "<td><form method='post'>",
  "<input class='btt' type='submit' name='listdir' value='List Dir(s)'>",
  "</form></td>",
  "@<td class='prjnam'> DevFsUploadESP - @0@ on ESP@2@</td>",
  "</tr></table><table><tr><td>"
};

// This code is based on original from ESP8266 documentation and webpage
// "A Beginner's Guide to the ESP8266" Chapter 11 SPIFFS, Uploading files to SPIFFS
//
// the code has been modified to provide only upload file to
// do development of applications SPIFFS http files more rapidly

DevFsUploadESP::DevFsUploadESP() {
  setDirSelected("/");
}

/**
   The upload dir: field contains the current directory "pointer" with in the FS
   and is dynamically set from the list dir(s) dialogue or manual typing into the
   dir: text box
*/
void DevFsUploadESP::setDirSelected(String setStr) {
  atatArr[DIR_SELECT_IDX] = setStr;
};


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

  // OPTIMIZATION   from ->  String ajaxAccess = String(access) + "ajax";
  String ajaxAccess = String(access);
  ajaxAccess += "ajax";
  
  atatArr[AJAX_HANDLER_ADDR] = ajaxAccess;
  
  //  record the server object for use within this class
  serverPabc = serverP;

  viewFileName = "";
  setDirSelected("/");

  // set handles for the various needs
  // 1) GET->initial load, 2a) POST->onFileUpLoad 2b) POST->List,..
  //
  //      2a implies POST action/form and once the upload is
  //      complete the regular POST action happens with no parameters
  //  
  serverPabc->on(access, HTTP_GET, DevFsUploadESP::handleUploadPage); // 1a
  serverPabc->onFileUpload(DevFsUploadESP::handleFileUpload);            // 2a
  serverPabc->on(access, HTTP_POST, DevFsUploadESP::handleOther);
  serverPabc->on(ajaxAccess, HTTP_POST, DevFsUploadESP::handleAjax);
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
  DevFsUploadESP::respondHttp200(client, true);
  DevFsUploadESP::doctypeBody(client);


  if (atatArr[TITLE_FS_TYPE_IDX] == "") {
    // set once only
    atatArr[TITLE_FS_TYPE_IDX] = String(DevFsUploadESP::projTitleTarget);
  }
  clientPrtLn(client, sizeof(mainPgArr) / sizeof(mainPgArr[0]), mainPgArr);

  if (uploadAction) {
    if (upldFileList != "") {
      client.println("<br>Last upload:");
      client.println(upldFileList);

      // reported once only
      upldFileList="";
      uploadStarted=false;
    }
    if (errUpl != "") {
      String s = "<div>";
      s += errUpl;
      s += errUplAdd;
      s += "</div>";

      client.println(s);

      // reported once
      errUpl = "";
      errUplAdd = "";
      uploadStarted = false;
    }
    uploadAction = false;
  }
  // zone for the LIST and LIST DIR contents table
  //? optimize the size of string array/const char*
  clientPrtLn(client, sizeof(mainPgListZoneArr) / sizeof(mainPgListZoneArr[0]), mainPgListZoneArr);

  // if there is a 'list' or the file system has been changed
  // update the list-files info
  //
  // "list files" info will be dynamically filled, or not
  if (listFilesAction) {
    //
    client.print("<div class='scrl' tabindex='-1'><div id='lsttab' class='bof dvtb'>");
    
    // The above information will be changed via a client javascript 'onload'
    // to a presentable HTML structure
    //
    DevFsUploadESP::listFilesOrDirsFSTargeted("/", client, true, true);

    client.print("</div></div>");
    listFilesAction = false;
  }
  client.println("</td><td>");
  
  // if there is a 'list dirs' or the file system has been changed
  // update the list-dirs info
  //
  // "list dir(s)" info will be dynamically filled, or not
  if (listDirsAction) {
    //
    client.print("<div class='scrl' tabindex='-1'><div id='lstdirtab' class='bof dvtb'>");
    
    // The above information will be changed via a client javascript 'onload'
    // to a presentable HTML structure
    //
    DevFsUploadESP::listFilesOrDirsFSTargeted("/", client, false, true);

    client.print("</div></div>");
    listDirsAction = false;
  }
  client.println("</td></tr></table>");

  if (viewFileName != "") {
    processViewFile();
    // reset the file being viewed
    viewFileName = "";
  }
  client.println("</body></html>");
  client.stop();
}

void DevFsUploadESP::respondHttp200(WiFiClient client, boolean htmlText) {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: ");
  if (htmlText) {
    client.println("text/html");
  } else {
    client.println("text/plain");
  }
  client.println("");
}

/**
   Send the doctype to body HTML code to the browser.

   This includes styles, scripts in the head and the body line
*/
void DevFsUploadESP::doctypeBody(WiFiClient client) {
    // header and <style> stuff
  clientPrtLn(client, sizeof(doctypeHTMLArr1) / sizeof(doctypeHTMLArr1[0]), doctypeHTMLArr1);
  
  // <script> stuff
  clientPrtLn(client, sizeof(doctypeHTMLArr2) / sizeof(doctypeHTMLArr2[0]), doctypeHTMLArr2);
}

/**
  Alter a string that contains @n@ patterns from the content of the
  atatArr elements.

  NB: not fault tolerate code if a string with @some words inbetween@ is in the
      altered parameter.
*/
String DevFsUploadESP::alterString(String altered) {
  // get the @ pairs that represent a replacement index
  while (true) {
    // if not @ pairs found, done the while
    int pos1stIndex = altered.indexOf("@");
    if (pos1stIndex == -1 ) break;

    int pos2ndIndex = altered.indexOf("@", pos1stIndex + 1);
    if (pos2ndIndex == -1 ) break;
      
    // pos1st...........V
    // pos2nd...............V
    // extract the key '@nn@' => 'nnn'
    //
    // substring---------A A     == pos1st_+1   pos2nd used as idx before
    //
    String key = altered.substring(pos1stIndex, pos2ndIndex + 1);
    long keyInt = key.substring(1, key.length() - 1).toInt(); // stoi(key);

    // this is a static association and no special subscriber-index
    // error checking is implemented
    altered.replace(key, atatArr[keyInt]);
  } // end while
  return altered;
}

/**
   Print a line to the WiFi client, with preprocessing for any @n@ processing.
   
   An example of a call to this method:
     clientPrtLn(client, sizeof(mainPgArr) / sizeof(mainPgArr[0]), mainPgArr);
   
   Note: 'sizeof' is compile-time calculation on a 'const char* arr[]' structure
         so attempting to calculate length-of-array at
         runtime does not work in C.   
*/
void DevFsUploadESP::clientPrtLn(WiFiClient client, int lenArr, const char* arr[]) {
  for (int i=0; i < lenArr; i++) {
    if (arr[i][0] == '@') {
      // need to alter the line for information
      String alterLineStr=arr[i];

      // remove the leading '@' and alter-the-string
      client.println(alterString(alterLineStr.substring(1)));

    } else {
      client.println(arr[i]);
    }
  }
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
  uploadAction=(serverPabc->args() == 0);

  if (uploadAction) {
    // assume on-file-upload has completed its request and
    // a regular POST happens to close off the upload action
    // which has 0 (zero) arguments
    upldFileList += "</table>";

  } else {
    // either list request overrides anything else 
    // ('list' are mutually exclusive)
    listFilesAction=serverPabc->hasArg("list");
    listDirsAction=serverPabc->hasArg("listdir");

    if (!listFilesAction && !listDirsAction) {
      // everything else
          
      // process file requests if the "fn" key/arg is present
      // process directory requests if the "dn" key/arg is present
      //
      if (serverPabc->hasArg("fn")) {
        //
        // 'List files' request kinds
        //
        if (serverPabc->hasArg("delete")) {
          deleteFileTargeted(serverPabc->arg("fn"));
          listFilesAction = true;

        } else if (serverPabc->hasArg("view")) {
          viewFileName=serverPabc->arg("fn");
        }

      } else if (serverPabc->hasArg("dn")) {
        //
        // List Dir(s) request kinds
        //
        if (serverPabc->hasArg("selectdir")) {
          listDirsAction = true;
          // set the selected directory to be the prefix for
          // files being uploaded (HTML file-upload does not
          // provide the hosts browsers directory paths, only
          // file names)
          setDirSelected(serverPabc->arg("fn"));

        } else if (serverPabc->hasArg("mksubdir")) {
          listDirsAction = true;

          if (serverPabc->hasArg("dnsub")) {
             if(serverPabc->arg("dnsub") != ""){
                // make the directory with the full-paths
                mkDirFSTargeted(serverPabc->arg("dn") + serverPabc->arg("dnsub"));
             }
          }

        } else if (serverPabc->hasArg("rmdir")) {
          listDirsAction = true;

          // remove the directory, but remove all internal files before hand
          // (NB: remove directory when the last file in its
          // structure is removed too)
          DevFsUploadESP::rmDirFSTargeted(serverPabc->arg("dn"));
        }
      } // end  hasArg("dn")/("fn")
    } // end (!listFilesAction && !listDirsAction)
  } // end uploadAction
  //
  // the settings of viewFileName, listDirsAction and listFilesAction will
  // cause processing in sendComplete to output the 'list' contents
  DevFsUploadESP::sendComplete();
}

/**
  process the file and return the content as text for viewing
*/
void DevFsUploadESP::processViewFile() {
  // open the file if possible
  File f = openFile4ReadTargeted(viewFileName);

  if (!f) {
    // nothing can be done if the file does not open
    return;
  }
  // process the file into a view-able textarea
  WiFiClient client = serverPabc->client();
  client.print("<br><p>Viewing: ");
  client.print(viewFileName);
  client.println("</p>");

  client.print("<textarea style='width:95%;' spellcheck='false' rows='15'>");
  int yieldPt = 100;
  while (f.available()) {
    client.print(f.readStringUntil('\r'));
    yieldPt--;
    if (yield == 0) {
      // a large file (not typical) could cause ESP Watch Dog Timer
      // issue thus a yield
      yield();
      yieldPt = 100;
    }
  }
  f.close();
  client.println("</textarea>");

  if (viewFileName.endsWith(".png") || viewFileName.endsWith(".jpg")
      || viewFileName.endsWith(".bmp") || viewFileName.endsWith(".gif")) {
    // if the file is an image then process into an img tag
    client.println("<img src='.." + viewFileName + "' alt='missing img'>");
  }
}
/**
  handleAjax call for the directory selected request
*/

void DevFsUploadESP::handleAjax() {

  if (serverPabc->hasArg("fn")) {
    if (serverPabc->hasArg("selectdir")) {
      listDirsAction = true;
      // set the selected directory to be the prefix for
      // files being uploaded (HTML file-upload does not
      // provide the hosts browsers directory paths, only
      // file names)
      setDirSelected(serverPabc->arg("fn"));
    }
  }
  WiFiClient client=serverPabc->client();
  DevFsUploadESP::respondHttp200(client, true);
  client.println("okay");
  client.stop();
}

/**
  upload a new file to the SPIFFS protocol handler
*/
void DevFsUploadESP::handleFileUpload() {
  HTTPUpload& upload = serverPabc->upload();
  uploadAction = true;

  // the HTTP upload protocol operates as a state machine
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
        DevFsUploadESP::handleUploadPage();
      } else {
        // If the file was successfully created
        fsUploadFile.close();
        uploadStarted = false;

        // OPTIMIZATION  from -> upldFileList += "<td>" + String(upload.totalSize) + "</td></tr>";
        upldFileList += "<td>";
        upldFileList += String(upload.totalSize);
        upldFileList += "</td></tr>";
      }

    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      Serial.print("Aborted: ");
      Serial.println(upload.filename);
    }
  } else {
    // file not open so expect an upload protocol error

    if (upload.status != UPLOAD_FILE_START) {
      if (errUpl == "") {
        errUpl = "500: protocol order issue";
      }
    } else {
      // dirSelected variable needs to prefix the file name
      // and it is assumed to be '/xxxxx/' in format (as per the protocol)
      String fn = atatArr[DIR_SELECT_IDX];
      fn += upload.filename;

      if (upldFileList == "") {
        upldFileList = lastFileTable1;
        upldFileList += lastFileTable2;
      }
      // OPTIMIZATION from -> upldFileList += "<tr><td>" + fn + "</td>";
      upldFileList += "<tr><td>";
      upldFileList += fn;
      upldFileList += "</td>";
      
      // Open the file for writing in SPIFFS/LittleFS and overwrite
      // (create if it doesn't exist)
      //
      fsUploadFile = DevFsUploadESP::openFile4WriteTargeted(fn);

      if (!fsUploadFile) {
        errUpl = "500: fail create file: ";
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

   