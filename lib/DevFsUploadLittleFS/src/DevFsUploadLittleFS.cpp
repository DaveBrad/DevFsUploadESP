//* Copyright (c) 2018-2020 dbradley. */

#ifdef ESP32
#error "LittleFS on ESP32 not fully support/implemented (in the ESP community)"
#endif
#ifndef ESP8266
#ifndef ESP32
#error "Only for ESP32/8266 module"
#endif
#endif



/**  License found at: 

Three other files should accompany this file so as to be valid.
'DevFsUpLoadLittleFS.h', along with 'DevFsUploadCommon.cpp' & 'DevFsUploadCommon.h'

The license description is contained with 'DevFsUploadCommon.h' along with usage
instructions.

If this files is not accompanied with the above 3 files and the license intact. This
file is suspect.

*/
#include <DevFsUploadCommon.h>
#include <DevFsUploadLittleFS.h>

#include <Arduino.h>
#include <FS.h>
#include <WiFiClient.h>

// the server object to operate through

#ifdef ESP32
#include <ESP32WebServer.h>
#include <LittleFS.h>

#else
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#endif

const char* DevFsUploadESP::projTitleTarget = "LittleFS";

void DevFsUploadESP::listFilesOrDirsFSTargeted(String dirname, WiFiClient client, boolean listFiles, boolean startingAtRoot) {
  // listing directories will have a root, so need to output it explicitly
  if (!listFiles) {
    if (startingAtRoot) {
      client.println("<div>/ 0</div>");
    }
  }
  // open a directory for processing and then process through the
  // contents to get files or directory information
  Dir root = LittleFS.openDir(dirname);

  while (root.next ()) {

    if (root.isFile()) {
      if (listFiles) {
        // gather the data about the file we need
        File filee = root.openFile("r");
        String fullName = filee.fullName();
        String size = String(filee.size());
        filee.close();
		
		if (! fullName.startsWith("/")) {
          // files in root are assumed to not have a leading '/'
          fullName = "/" + fullName;
        }
        // format the data into HTML format
        String str = "<div>" + fullName + " " + size + "</div>";
        client.println(str);
		
		// a lot of files (not typical) could cause ESP Watch Dog Timer issue
        // thus a yield
        yield();
      }
    } else if (root.isDirectory()) {
      // unfortunately LittleFS does not provide a full-name/full-path
      // method for directories, so the following code emulates
      // full-path construction
      String dirNam = String(dirname);

      if (dirNam != "/") {
        dirNam += "/";
      }
      dirNam += root.fileName();

      if (!listFiles) {
        // processing the directories to get a listing of directories
        // to the HTML UI
        String str = "<div>" + dirNam  + "/ 0</div>";
        client.println(str);
      }
      // process the sub-directory
      DevFsUploadESP::listFilesOrDirsFSTargeted(dirNam, client, listFiles, false);
    }
  }
}

// LittleFS
void DevFsUploadESP::mkDirFSTargeted(String nuDirFullPath){
	LittleFS.mkdir(nuDirFullPath);
}

// LIttleFS
File DevFsUploadESP::openFile4WriteTargeted(String fn){
	return LittleFS.open(fn, "w");
}
	
// LIttleFS 
File DevFsUploadESP::openFile4ReadTargeted(String fn){
	return LittleFS.open(viewFileName, "r");
}

// LittleFS
void DevFsUploadESP::deleteFileTargeted(String fn){
    if (LittleFS.exists(fn)) {
      LittleFS.remove(fn);
    }
}

// LittleFS
void DevFsUploadESP::rmDirFSTargeted(String dirname) {
  // LittleFS provides rmdir, but it needs the directory to be
  // empty
  // open a directory for processing and then process through the
  // contents to get files or directory information
  Dir root = LittleFS.openDir(dirname);

  while (root.next ()) {
    if (root.isFile()) {
      // gather the data about the file we need
      File filee = root.openFile("r");
      String fullName = filee.fullName();
      filee.close();

      LittleFS.remove(fullName);

      // // format the data into HTML format
      // String str = "<div>";

      // if(! fullName.startsWith("/")){
      // // files in root are not assumed to have a leading '/'
      // str += "/";
      // }
      // str += fullName + " " + size + "</div>";
      // client.println(str);
    } else if (root.isDirectory()) {
      // unfortunately LittleFS does not provide a full-name/full-path
      // method for directories, so the following code emulates
      // full-path construction
      String dirNam = String(dirname);

      if (dirNam != "/") {
        dirNam += "/";
      }
      dirNam += root.fileName();

      // process the sub-directory
      DevFsUploadESP::rmDirFSTargeted(dirNam);

      // LittleFS automatically deletes the directory when it becomes
      // empty, so no explicit action is required.
    }
  }
  LittleFS.rmdir(dirname);
}
