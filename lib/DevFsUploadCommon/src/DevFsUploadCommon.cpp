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

// selected-dir in the upload dir: field, used for sync of setting
// between the Browser-UI and the ESP
String DevFsUploadESP::selectedDir = "/";    // starts at the root

// SPIFFS/LittleFS file names may not exceed 32 chars [31 + \0] of a char string
File DevFsUploadESP::fsUploadFile;

String DevFsUploadESP::upldFileList = "";
char* DevFsUploadESP::errUpl = "";
String DevFsUploadESP::errUplAdd = "";

const char* DevFsUploadESP::lastFileTable1 = "<style>.tab td:nth-child(2){text-align: right;}</style>";

const char* DevFsUploadESP::lastFileTable2 = "<table class='tab'><tr><th>File name</th><th>Size</th></tr>";

const char* DevFsUploadESP::uploadAccessDefault = "/upload";

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
.none{display:none}\
.btt{border-radius:15px; display:inline-block;}\
.btt:focus{border:2px solid blue;}\
.btt2{color:red;}\
.btt3{width:0px;height:18px; padding-right:15px;background:#f2fcf9;}\
.info{font-style:italic;font-size:0.8em;}\
.bof{direction:ltr;float:left;}\
.scrl{margin-left:24px;height:155px;overflow-y:scroll;direction:rtl;}\
.dvtb{display:table;}\
.dvtb .dvro:nth-child(3n) div,\
.dvtb .dvro:nth-child(3n) div input{background:#ffffcc;}\
.sudo,\
.dvtb .dvro:nth-child(3n) div .sudo{font-style:italic;background:#8fd8d8;}\
\
.dvro{display:table-row;}\
.dvro div:nth-child(1) input{border:0px;}\
\
.dvcl{display:table-cell;padding-right:10px;}\
.dnfrm{display:inline;}\
\
.siz{text-align:right;}\
.prjnam{color:#ff4500;text-decoration:underline;padding-bottom:10px;}\
#vuid{width:85ch;}\
#vuid p{margin-top:0;}\
</style>"
};

const char* DevFsUploadESP::doctypeHTMLArr2[] = {
  /**
      begin the processing of scripts
  */
  "<script>",

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
for(i=0;i<arr.length;i++){\
var s=arr[i];\
var idx=s.lastIndexOf(' ');\
var nm=s.substring(0,idx);\
var len=nm.length+2;\
\
var siz=s.substring(idx+1);\
\
s=\"<div class='dvro'><div class='dvcl'><input \";\
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
<div class='dvcl siz'>\"+siz+\"</div><div class='dvcl'>\
<button class='btt' onclick=\\\"ajaxFn('delete', this);\\\">Delete</button>\
<button class='btt' onclick=\\\"ajaxFn('view', this);\\\">View</button>\";\
\
s+=\"<form class='dnfrm' method='post'>\
<input type='hidden' name='fn' readonly value='\"+nm+\"'>\
<button class='btt' type='submit' name='down'>Download</button>\
</form></div>\";\
\
}else{",
  // dir-table content
  //
"s+=\"><button class='btt' name='seldir' onclick=\\\"setDf('\"+nm+\"');\\\">Select</button>\
</div>\";",

  // mk-subdir & rmdir
  //
  // NB: coding style using '\' optimizes both ESP program storage and global
  //     variable use. Retain the style and check if making changes for increased
  //  storage usage.
"s+=\"<div class='dvcl'>\
<input type='text' autocomplete='off' name='dnsub' value='' tabindex='-1' oninput='mkval(this);'>\
<button class='btt' disabled onclick=\\\"ajaxMkdr(this);\\\">mk-subDir</button>\
<button class='btt btt2' onclick=\\\"ajaxRmdir(this);\\\">rmDir</button>\
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
    function clrLstUp (clear-Last-Uploaded) will clear the last upload zone of information
    as any ajax action will cause this to be redundant.
  */
  "function clrLstUp(){\
var e = gE('lastupldid');\
if(e !== null) e.remove();\
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
if(action !== 'onload')clrLstUp();\
\
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
h.open('POST','ajaxESP',false);\
h.setRequestHeader('Content-type','application/x-www-form-urlencoded');\
\
h.onreadystatechange = function() {\
if (this.readyState===4 && this.status===200) {\
\
var respStr=this.responseText;\
\
if(respStr.startsWith('okay')) return;\
\
\
if(respStr.startsWith('onload=')){\
respStr = respStr.substring(7);\
var arr = respStr.split(',');\
gE('fstyid').innerHTML=arr[0];\
gE('dirroot').value=arr[1];\
\
gE('bdy').classList.remove('none');\
return;\
}\
\
respStr=respStr.replace(/\\r|\\n/g, '');\
\
var filesLst=gE('lflsid');\
filesLst.innerHTML='';\
\
var dirsLst=gE('ldrsid');\
dirsLst.innerHTML='';\
\
var viewFld=gE('vuid');\
viewFld.innerHTML='';\
\
if(respStr.startsWith('lfiles')){\
filesLst.innerHTML=respStr.substring(6);\
sortList(true);\
\
}else if(respStr.startsWith('ldirs')){\
dirsLst.innerHTML=respStr.substring(5);\
sortList(false);\
\
}else if(respStr.startsWith('<p>Viewing')){\
viewFld.innerHTML=respStr;\
\
}\
}\
};\
h.send(p);\
}",

/**
   function bodyLo (bodyLoaded) is a onload action for the body
   
   Pieces of data are required to complete the page
   1) the upload dir: needs to be set as per the selectedDir on the ESP
      (so as to maintain sync between the Browser-UI and ESP)
   2) The file-system type being used, which needs to be included
      when the page has finished loading (this also performs
      the release display: none on the <body>
*/
"function bodyLo(){ajax('onload');}",


  /* done scripts so other scripts are added prior */
  "</script></head><body id='bdy' class='none' onload='bodyLo();'>"

};

/**
  mainPgxxxxxxxArr arrays contain the HTML code for the main-page of DevFsUploadESP.

*/
// browse buttons
const char* DevFsUploadESP::mainPgArr[] = {
  // title zone
  "<table>",

  // - browse button zone
  // -- directory selected info, input
  "<tr><td>1.</td>",
  "<td colspan='2' style='text-align:center'>\
upload dir: <input type='text' id='dirroot' name='dirroot' oninput='oninpDir();' \
value='' size='32'><span class='info'> Select dir from 'List Dir...'</span></td></tr>",

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
  // - view zone

  "<table><tr>",  // list control buttons
  "<td><button class='btt' onclick=\"ajax('list', 'files', 'do');\">List Files</button>",
  "<button class='btt' onclick=\"ajax('list', 'dirs', 'do');\">List Dir(s)</button></td>",
#ifdef ESP32
  "<td class='prjnam'> DevFsUploadESP - <span id='fstyid'></span> on ESP32</td></tr></table>",
#else
  "<td class='prjnam'> DevFsUploadESP - <span id='fstyid'></span> on ESP8266</td></tr></table>",
#endif
  // ---- list files & dir(s) content container
  "<div id='lflsid'></div>",   // list files content
  "<div id='ldrsid'></div>",   // list dirs content

  // ---- view container
  "<table><tr><td><div id='vuid'></div></td></tr></table>", // view zone
  "</body></html>"
};


 // // ---- list files & dir(s) content container
  // "<table><tr><td id='lflsid'></td>",   // list files content
  // "<td id='ldrsid'></td></tr></table>",   // list dirs content
  
  


 // // ---- list files & dir(s) content container
  // "<table><tr><td><div class='scrl' tabindex='-1'><div id='lflsid'></div></div></td>",   // list files content
  // "<td><div class='scrl' tabindex='-1'><div id='ldrsid'></div></div></td></tr></table>",   // list dirs content

  // // ---- view container
  // "<table><tr><td><div id='vuid'></div></td></tr></table>", // view zone
  // "</body></html>"


// <div class='scrl' tabindex='-1'><div id='lsttab' class='bof dvtb'>
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
  DevFsUploadESP::respondHttp200(client, true);
  DevFsUploadESP::doctypeBody(client);

  // do we have a simple error condition to report
  if (simpleErrorMsg != "") {
    client.println(simpleErrorMsg);
    simpleErrorMsg = "";

  } else {
    clientPrtLn(client, sizeof(mainPgArr) / sizeof(mainPgArr[0]), mainPgArr);

    if (uploadAction) {
      if (upldFileList != "") {
        client.println("<div id='lastupldid'><br>Last upload:");
        client.println(upldFileList);
        client.println("</div>");

        // reported once only
        upldFileList = "";
        uploadStarted = false;
      }
      if (errUpl != "") {
        String s = "<div id='errupld'>";
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
    clientPrtLn(client, sizeof(mainPgListZoneArr) / sizeof(mainPgListZoneArr[0]), mainPgListZoneArr);
  }
  client.stop();
}

void DevFsUploadESP::respondHttp200(WiFiClient client, boolean htmlText) {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: text/");
  if (htmlText) {
    client.println("html");
  } else {
    client.println("plain");
  }
  client.println("Connection: close");
  client.println("");
}

void DevFsUploadESP::respondHttp200(WiFiClient client, boolean htmlText, int length) {
  client.println("HTTP/1.1 200 OK");
  client.print("Content-Type: text/");
  if (htmlText) {
    client.println("html");
  } else {
    client.println("plain");
  }
  client.print("Content-Length:");
  client.println(length);
  
   // client.println("Connection:keep-alive");
   
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
   Print a line to the WiFi client.

   An example of a call to this method:
     clientPrtLn(client, sizeof(mainPgArr) / sizeof(mainPgArr[0]), mainPgArr);

   Note: 'sizeof' is compile-time calculation on a 'const char* arr[]' structure
         so attempting to calculate length-of-array at
         runtime does not work in C.
*/
void DevFsUploadESP::clientPrtLn(WiFiClient client, int lenArr, const char* arr[]) {
  for (int i = 0; i < lenArr; i++) {
      client.println(arr[i]);
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
  client.print("<p>Viewing: ");
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
  // may download the file, but provide filename without path
  String fnNoPath = fn.substring(idx + 1);

  serverPabc->sendHeader("Content-Type", "text/text");
  serverPabc->sendHeader("Content-Disposition", "attachment; filename=" + fnNoPath);
  serverPabc->sendHeader("Connection", "close");
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

void DevFsUploadESP::handlerAjax() {

  
  WiFiClient client = serverPabc->client();
  
  
  String action = serverPabc->arg("act");
 
  
  // the FS type for the title is returned as it applies to onload
  if(action == "onload"){
      // client.print(action);
      //
      // this is a request for information that is a strict format
      // expectation
     String sendStr = "onload=";
     sendStr += String(projTitleTarget) + ",";
     sendStr += String(selectedDir);
     
     serverPabc->send(200, "text/plain", sendStr);
     client.stop();
     return;
  }
  //?
  DevFsUploadESP::respondHttp200(client, false);  // false-plain text
    
  // assume list-files and list-dirs are assumed to not be refreshed
  // HOWEVER, if the list-file or list-dirs action (eg. Delete file) 
  // affects the contents of the list-xxxx, then set for refresh in the
  // conditional code below
  boolean listFilesAffected = false;
  boolean listDirsAffected = false;
  
   // file requests will have an arg of 'fn'
  if (serverPabc->hasArg("fn")) {
    
    if (action == "delete") {
      deleteFileTargeted(serverPabc->arg("fn"));
      listFilesAffected = true;

    } else if (action == "view") {
      processViewFile(serverPabc->arg("fn"));
    }
    // download is excluded from ajax as its a "separate" ajax type
    // of connection via HTML-HTTP protocol
        
  } else if (serverPabc->hasArg("dn")) {
    // directory actions

    if (action == "seldir") {
      listDirsAction = true;
      // set the selected directory to be the prefix for
      // files being uploaded (HTML file-upload does not
      // provide the hosts browsers directory paths, only
      // file names)
      selectedDir = serverPabc->arg("dn");
      client.println("okay");

    } else

      if (action == "mksubdir") {
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
      }

  } else if (action == "list") {

    // list request
    if (serverPabc->hasArg("files")) {
      listFilesAffected = true;

    } else if (serverPabc->hasArg("dirs")) {
      listDirsAffected = true;
    }
  }
  // list either files or dir(s) as appropriate (they are mutually exclusive)
  if(listFilesAffected || listDirsAffected){
      const char* listKind = (listFilesAffected ? "lfiles" : "ldirs");
      client.print(listKind);
      
      client.print("<div class='scrl' tabindex='-1'><div  class='bof dvtb' id='");
      client.print(listKind);
      client.print("'>");
          
      // starting at root and listing directory will need the implied '/'
      // explicitly required
      if(!listFilesAffected) client.println("<div>/ 0</div>");
          
      DevFsUploadESP::listFilesOrDirsFSTargeted("/", client, listFilesAffected);
      client.print("</div></div>");
  }
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
      String fn = selectedDir;
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

