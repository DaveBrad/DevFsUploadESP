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

const char*  DevFsUploadESP::projEspTarget PROGMEM = " on ESP32";
#else
#include <ESP8266WebServer.h>

ESP8266WebServer* DevFsUploadESP::serverPabc;

const char* PROGMEM DevFsUploadESP::projEspTarget PROGMEM = " on ESP8266";

#endif

// selected-dir in the upload dir: field, used for sync of setting
// between the Browser-UI and the ESP
String DevFsUploadESP::selectedDir = "/";    // starts at the root

// SPIFFS/LittleFS file names may not exceed 32 chars [31 + \0] of a char string
File DevFsUploadESP::fsUploadFile;
boolean DevFsUploadESP::uploadStarted = false;
boolean DevFsUploadESP::uploadAction = false;

String DevFsUploadESP::upldFileList = "";
char* DevFsUploadESP::errUpl = "";
String DevFsUploadESP::errUplAdd = "";

const char* PROGMEM DevFsUploadESP::uploadAccessDefault = "/upload";

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

int DevFsUploadESP::mainPageLength = -1;;
int DevFsUploadESP::mainPageArrSize = -1;;

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
const char* DevFsUploadESP::pageHTMLArr[]  = {
  /* Arduino IDE places this into PROGMEM without the keyword */

  "<!DOCTYPE HTML><html lang='en'><head><meta charset='UTF-8'>",

  /**
    NB: coding style using '\' optimizes both ESP program storage and global
        variable use. Retain the style and check if making changes for increased
        storage usage.

        ->  use color code to save 2 bytes vs name
        orangered  #ff4500
        lightblue  #8fd8d8

        b  b2 b3        button
        dtb dro dcl     div table row cell
        foc             focus element that is not a button
        brw             browse+ button-field from multiple file/dir upload

  */
  "<title>DevFsUploadESP v 2</title>",
  "<style>\
.none{display:none}\
.b{border-radius:15px;display:inline;}\
.foc{border:2px solid lightgray;}\
.brw{border:2px solid white;}\
.foc:focus,.brw:focus,.b:focus{border:2px solid blue;}\
.b2{color:red;}\
.b3{width:0px;height:18px;padding-right:15px;background:#f2fcf9;}\
.info{font-style:italic;font-size:0.8em;}\
.bof{direction:ltr;float:left;}\
.scrl{margin-left:24px;height:155px;overflow-y:scroll;direction:rtl;}\
.dtb{display:table;}\
.dtb .dro:nth-child(3n) div,\
.dtb .dro:nth-child(3n) div input{background:#ffffcc;}\
.sudo,\
.dtb .dro:nth-child(3n) div .sudo{font-style:italic;background:#8fd8d8;}\
\
.dro{display:table-row;}\
.dro div:nth-child(1) input{border:0px;}\
\
.dcl{display:table-cell;padding-right:10px;}\
.dnfrm{display:inline;}\
\
.siz{text-align:right;}\
.prjnam{color:#ff4500;text-decoration:underline;padding-bottom:10px;}\
#vuid{width:85ch;}\
#vuid p{margin-top:0;}\
\
.tab td:nth-child(2){text-align: right;}\
</style><script>",
  /**
      begin the processing of scripts
  */

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
  "function resetF(formid){gE(formid).reset();upldbts();}",

  /**
    Sorts the file-list or file-dir(s) table alphabetically for display.

    Also, inserts HTML code for controls based on raw data
    from the ESP-server for Browser-UI presentation.

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.

    isF   is-file-Boolean    true if for files, false for directories listing
  */
  "function sortList(isF){\
var tab=gE(isF ? 'lfiles':'ldirs');\
if(tab===null)return;\
\
var arr=[];\
while(tab.firstChild){\
var a=tab.firstChild.innerHTML;\
if(a!==undefined)arr.push(a);\
tab.removeChild(tab.firstChild);\
}\
\
arr.sort();\
for(var i=0;i<arr.length;i++){\
var s=arr[i];\
var idx=s.lastIndexOf(' ');\
var nm=s.substring(0,idx);\
var len=nm.length+2;\
\
var siz=s.substring(idx+1);\
\
s=\"<div class='dro'><div class='dcl'><input \";\
\
if(isF && nm.endsWith(\"/\"\)\
|| !isF && siz===\"1\"){\
s+=\"class='sudo' \";}\
\
s+=\"type='text' readonly tabindex='-1' value='\"+nm+\"'\";\
if(isF){",
  // file-table content
  //
  "s+=\" size='\"+len+\"'></div>\
<div class='dcl siz'>\"+siz+\"</div><div class='dcl'>\
<button class='b' onclick=\\\"ajaxFn('delete', this);\\\">Delete</button>\
<button class='b' onclick=\\\"ajaxFn('view', this);\\\">View</button>\";\
\
s+=\"<form class='dnfrm' method='post'>\
<input type='hidden' name='fn' readonly value='\"+nm+\"'>\
<button class='b' type='submit' name='down'>Download</button>\
</form></div>\";\
\
}else{",
  // dir-table content
  //
  "s+=\"><button class='b' name='seldir' onclick=\\\"setDf('\"+nm+\"');\\\">Select</button>\
</div>\";",

  // mk-subdir & rmdir
  //
  // NB: coding style using '\' optimizes both ESP program storage and global
  //     variable use. Retain the style and check if making changes for increased
  //  storage usage.
  "s+=\"<div class='dcl'>\
<input class='foc' type='text' autocomplete='off' name='dnsub' value='' tabindex='0' oninput='mkval(this);'>\
<button class='b' disabled onclick='ajaxMkdr(this);'>mk-subDir</button>\
<button class='b b2' onclick='ajaxRmdir(this);'>rmDir</button>\
</div>\";\
}\
tab.insertAdjacentHTML('beforeend', s);\
}\
}",     // end function

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
    function for setDf (set dir field) which sets the select-dir field (dir:) and
    performs ajax action to the upload-server

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function setDf(dn){\
ajax('seldir','dn',dn);\
gE('dirroot').value=dn;\
}",

  /**
    function for inpE (get input element) which gets the input element that holds the file-name or
    directory-path in the 'List Files' or 'List Dir(s)' display.

    NB: The structure of the display is based on element-node position to (this-Element).

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.

    thisE   thisEle   this-element

    return the element that is the input-element with filename or dir-path
  */
  "function inpE(thisE){return thisE.parentNode.parentNode.childNodes[0].childNodes[0];}",

  /**
     function for ajaxFn (ajax for filename (fn) processing) performs an action for a 'List Files'
     filename.

     NB: coding style using '\' optimizes both ESP program storage and global
       variable use. Retain the style and check if making changes for increased
     storage usage.

     Code has been really compressed to save storage space.

     act     action    string of the action to be performed (view, delete)
     thisE   thisEle   this-element
  */
  "function ajaxFn(act,thisE){ajax(act,'fn',inpE(thisE).value);}",

  /**
      function for ajaxMkdr (ajax make-directory [mkdir]) perform directory (dn) make directory
      as per the 'List Dir(s)' display button information.

      NB: The structure of the display is based on element-node position to (this-Element).

      NB: coding style using '\' optimizes both ESP program storage and global
        variable use. Retain the style and check if making changes for increased
      storage usage.

      Code has been really compressed to save storage space.

      thisE   thisEle   this-element
  */
  "function ajaxMkdr(thisE){\
ajax('mksubdir',\
'dn',inpE(thisE).value,\
'dnsub',thisE.previousSibling.value);\
}",


  /**
    function for ajaxRmdir (ajax remove-directory [rmdir]) perform directory (dn) remove-directory
    as per the 'List Dir(s)' display button information: with confirm prompt on the browser-UI.

    NB: The structure of the display is based on element-node position to (this-Element).

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.

    thisE   thisEle   this-element
  */
  "function ajaxRmdir(thisE){\
var dn=inpE(thisE).value;\
\
if(confirm(dn+\"\\n\\nrmDir not reverse-able\")) ajax('rmdir','dn',dn);\
}",


  /**
    function for mkval (make-sub-directory validate) for the mk-subdir input field.

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
if(ss) alert(em+\"  not permitted\");\
\
if(vE===\"\") ss=true;\
\
ele.nextElementSibling.disabled=ss;\
}",

  /**
    function onipnDir (on-input-directory) validation when the dir: field changes.Ensures
    the ESP and Browser-UI are in sync for a valid directory path format.

    NB: coding style using '\' optimizes both ESP program storage and global
     variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.
  */
  "function oninpDir(){\
var upFld=gE('dirroot');\
upldbts();\
\
if(upFld.value.endsWith(\"/\")) setDf(upFld.value);\
}",

  /**
     Function ajax [handlerAjax on ESP interface] will process changes to the selected-dir
     field and AJAX the info to the ESP server via XMLHttpRequest

    NB: coding style using '\' optimizes both ESP program storage and global
      variable use. Retain the style and check if making changes for increased
    storage usage.

    Code has been really compressed to save storage space.

    Do nothing on return from the server

    ????

    h http-object
    p params
  */
  "function ajax(action,p1='',v1='',p2='',v2=''){\
var h=new XMLHttpRequest();\
var p='act';\
p+= '=';\
p+=action;\
if(p1!==\"\"){\
p+='&';\
p+=p1;\
p+='=';\
p+=v1;\
}\
if(p2!==\"\"){\
p+='&';\
p+=p2;\
p+='=';\
p+=v2;\
}\
\
h.open('POST','ajaxESP',true);\
h.setRequestHeader('Content-type','application/x-www-form-urlencoded');\
\
h.onreadystatechange = function() {\
if (this.readyState===4 && this.status===200) {\
\
if(this.responseText.startsWith('okay')) return;\
\
['lastupldid', 'lflsid', 'ldrsid', 'vuid'].forEach(function(item){\
gE(item).innerHTML='';\
});\
\
var firstEq = this.responseText.indexOf(\"=\");\
var act = this.responseText.substring(0, firstEq);\
var resp = this.responseText.substring(firstEq + 1).replace(/\\r|\\n/g,'');\
\
switch(act){\
case 'onload':\
var arr=resp.split(',');\
\
gE('fstyid').innerHTML=arr[0];\
gE('dirroot').value=arr[1];\
\
if(arr[2]==='yesupld'){\
ajax('upldlst');\
}else{\
gE('bdy').classList.remove('none');\
}\
return;\
\
case 'lfiles':\
gE('lflsid').innerHTML=resp;\
sortList(true);\
break;\
\
case 'ldirs':\
gE('ldrsid').innerHTML=resp;\
sortList(false);\
break;\
\
case 'upldlist':\
gE('lastupldid').innerHTML=resp;\
gE('bdy').classList.remove('none');\
break;\
\
case 'view':\
gE('vuid').innerHTML=resp;\
break;\
}\
}\
};\
h.send(p);\
}",

  /* done scripts so other scripts are added prior */
  "</script></head><body id='bdy' class='none' onload='ajax(\"onload\");'>",

  // };

  /**
    mainPgxxxxxxxArr arrays contain the HTML code for the main-page of DevFsUploadESP.

  */
  // browse buttons
  // const char* PROGMEM DevFsUploadESP::mainPgArr[]  = {
  // title zone
  "<table>",

  // - browse button zone
  // -- directory selected info, input
  "<tr><td>1.</td>\
<td colspan='2' style='text-align:center'>\
upload dir: <input type='text' id='dirroot' class='foc' name='dirroot' oninput='oninpDir();' value='' size='32'><span class='info'> Select dir from 'List Dir...'</span></td></tr>\
\
<tr><td>2.</td><td><form method='post' id='upfile' enctype='multipart/form-data'>\
<button class='b b3' type='button' onclick='resetF(\"upfile\");'>R</button>\
<input type='file' id='upfileinp' class='brw' name='name' multiple onchange='upldbts();'><br>\
<button class='b' id='upfilebtt' disabled>Upload file(s)</button> to dir",
  "</form></td>\
\
<td> <form method='post' id='updir' enctype='multipart/form-data'>\
<button class='b b3' type='button' onclick='resetF(\"updir\");'>R</button>\
<input type='file' id='updirinp' class='brw' name='name' multiple webkitdirectory mozdirectory onchange='upldbts();' ><br>\
<button class='b' id='updirbtt' disabled>Upload Directory</button> all files: browse-dir + sub-dir(s)\
</form></td></tr>\
\
<tr><td></td><td colspan='2' class='info'>1-20 files ideal to prevent memory issues in upload execution on device.</td></tr></table>\
<div id='lastupldid'></div>\
\
\
<table><tr><td>\
<button class='b' onclick=\"ajax('list','files','do');\">List Files</button>\
<button class='b' onclick=\"ajax('list','dirs','do');\">List Dir(s)</button></td>\
\<td class='prjnam'> DevFsUploadESP - <span id='fstyid'></span></td></tr></table>\
\
<div id='lflsid'></div>\
<div id='ldrsid'></div>\
<table><tr><td><div id='vuid'></div></td></tr></table>", // view zone

  "</body></html>"
};

//
// This code is based on original from ESP8266 documentation and webpage
// "A Beginner's Guide to the ESP8266" Chapter 11 SPIFFS, Uploading files to SPIFFS
//
// the code has been modified to provide only upload file to
// do development of applications SPIFFS http files more rapidly

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
  // DevFsUploadESP::respondHttp200(client, true);
  // DevFsUploadESP::doctypeBody(client);

  // do we have a simple error condition to report
  if (simpleErrorMsg != "") {
    serverPabc->send(200, "text/plain", simpleErrorMsg);
    // client.println(simpleErrorMsg);
    simpleErrorMsg = "";

  } else {
    mainPage(client);

    if (uploadAction) {
      uploadStarted = false;
      uploadAction = false;
    }
  }
  client.stop();
}

void DevFsUploadESP::mainPage(WiFiClient client) {
  // the main page is a static page in a const char* xxxxx[]. Need
  // to calculate the size that is going to be used as the content-length
  // response in for the header
  //
  // this is only done once as the page is static
  if (mainPageArrSize == -1) {
    mainPageArrSize = sizeof(pageHTMLArr) / sizeof(pageHTMLArr[0]);

    mainPageLength = 0;
    for (int i = 0; i < mainPageArrSize; i++) {
      mainPageLength += strlen(pageHTMLArr[i]);   // client.println adds + 2
      mainPageLength += 2;
    }
  }
  // although ESP32/ESP8266-WebServer libraries have functions to
  // send and the such, the lbraries are very simple and as such
  // lack more capabilities on them

  respondHttp200(client, true, mainPageLength, false);

  for (int i = 0; i < mainPageArrSize; i++) {
    client.println(pageHTMLArr[i]);
  }
  client.stop();
}

/**
     Constructs the 200 response and supporting header information to
     be sent.
     client     is the client object to communicate with
     htmlText   true - will be text/html, false - text/plain content-type
     length     length in bytes of content that will be provided
     connectClose  default true: add to header the Connection: close
*/
void DevFsUploadESP::respondHttp200(WiFiClient client, boolean htmlText, int length, boolean connectionClose = false) {

  // although ESP32/ESP8266-WebServer library's have functions to
  // send and such, lack more involved capabilities to do responses
  // that are dynamic

  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: text/");
  client.println( htmlText ? "html" : "plain");

  if (length != 0) {
    client.print("Content-Length: ");
    client.println(length);
  }

  if (connectionClose) {
    client.println("Connection: close");
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

        simpleErrorMsg = "Download failure";
      }
    }
  } // end uploadAction
  DevFsUploadESP::sendComplete();
}

/**
  process the file and return the content as text for viewing
*/
void DevFsUploadESP::processViewFile(String filePath) {
  // open the file if possible
  File f = openFile4ReadTargeted(filePath);

  if (!f) {
    // nothing can be done if the file does not open
    return;
  }
  // process the file into a view-able textarea
  WiFiClient client = serverPabc->client();
  client.print("view=<p>Viewing: ");
  client.print(filePath);
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

  if (filePath.endsWith(".png") || filePath.endsWith(".jpg")
      || filePath.endsWith(".bmp") || filePath.endsWith(".gif")) {
    // if the file is an image then process into an img tag
    client.println("<img src='.." + filePath + "' alt='missing img'>");
  }
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
  String fnNoPath = fn.substring(idx + 1);

  serverPabc->sendHeader("Content-Type", "text/text");
  serverPabc->sendHeader("Content-Disposition", "attachment; filename=" + fnNoPath);

  serverPabc->streamFile(f, "application/octet-stream");
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
    // const char* prefix = "upldlist=<br>Last upload:";
    arrHolder[iOfArrs] = "upldlist=<br>Last upload:";
    iOfArrs++;

    content = upldFileList;

    arrHolder[iOfArrs] = content.c_str();
    iOfArrs++;

    // calculate the length of the message to be sent
    // int contentLen = strlen(prefix);
    // contentLen += upldFileList.length();
    if (errUpl != "") {
      content += "<div id='errupld'>";
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
    respondHttp200(client, false, -1);
    processViewFile(serverPabc->arg("fn"));
    client.stop();
    return;

    // download is excluded from ajax as its a "separate" ajax type
    // of connection via HTML-HTTP protocol

    // directory actions

  } else if (action == "mksubdir") {
    listDirsAffected = true;

    if (serverPabc->hasArg("dnsub")) {
      if (serverPabc->arg("dnsub") != "") {
        // make the directory with the full-paths
        mkDirFSTargeted(serverPabc->arg("dn") + serverPabc->arg("dnsub"));
      }
    }

  } else if (action ==  "rmdir") {
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
    }
  }
  // list either files or dir(s) as appropriate (they are mutually exclusive)
  if (listFilesAffected || listDirsAffected) {
    // the key for the ajax response

    content = listFilesAffected ? "lfiles" : "ldirs";
    content += "=<div class='scrl' tabindex='-1'><div class='bof dtb' id='";
    content += listFilesAffected ? "lfiles'>" : "ldirs'><div>/ 0</div>";

    // put in place any other found directories or files
    content += DevFsUploadESP::listFilesOrDirsFSTargeted("/", listFilesAffected);

    arrHolder[iOfArrs] = content.c_str();
    iOfArrs++;

    arrHolder[iOfArrs] = "</div></div>";
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
  respondHttp200(client, false, totalLen);
  // now do the output to the client
  for (int i = 0; i < iOfArrs; i++) {

    //d Serial.println(arrHolder[i]);
    client.println(arrHolder[i]);
  }
  client.stop();
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
        serverPabc->send(200, "text/plain", "done");

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
      String fn = selectedDir;
      fn += upload.filename;

      if (upldFileList == "") {
        upldFileList = "<table class='tab'><tr><th>File name</th><th>Size</th></tr>";
      }
      // OPTIMIZATION from -> upldFileList += "<tr><td>" + fn + "</td>";
      upldFileList += "<tr><td>";
      upldFileList += fn;
      upldFileList += "</td>";

      // Open the file for writing in SPIFFS/LittleFS and overwrite
      // (create if it doesn't exist)
      //
      // leave open as its closed on the "state-machine" UPLOAD_FILE_END state
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

