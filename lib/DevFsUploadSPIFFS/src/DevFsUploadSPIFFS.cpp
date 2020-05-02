/* Copyright (c) 2018-2020 dbradley. */
#ifndef ESP8266
#ifndef ESP32
#error "Only for ESP32/8266 module"
#endif
#endif

/**  License found at: 

Three other files should accompany this file so as to be valid.
'DevFsUpLoadSPIFFS.h', along with 'DevFsUploadCommon.cpp' & 'DevFsUploadCommon.h'

The license description is contained with 'DevFsUploadCommon.h' along with usage
instructions.

If this files is not accompanied with the above 3 files and the license intact. This
file is suspect.

*/
#include <DevFsUploadCommon.h>
#include <DevFsUploadSPIFFS.h>

#ifdef ESP32
// #include <ESP32WebServer.h>
#include <SPIFFS.h>

#else
// #include <ESP8266WebServer.h>

#endif

const char* DevFsUploadESP::projTitleTarget = "SPIFFS";

String DevFsUploadESP::listFilesOrDirsFSTargeted(String dirname, boolean listFiles) {

  String content = "";
  // open a directory for processing and then process through the
  // contents to get files or directory information
#define MAX_DIR_SUPPORTED 100
  String dirArr[MAX_DIR_SUPPORTED]; // assume less than 100 directories
  boolean dirPseudoArr[MAX_DIR_SUPPORTED];
  dirArr[0] = "/";
  dirPseudoArr[0] = false;

  int dirArrNextIdx = 1;

  // ***********************
  // SPIFFS on ESP32 and ESP8266 are different at the openDir level and
  // processing of files. Extract the data needed and then process
  // in a common manner.
  //
#ifdef ESP32
  File filee;
  File root = SPIFFS.open("/");
  
  while (filee = root.openNextFile()) {
    String fullName = String(filee.name());
    String size = String(filee.size());

#else
  Dir root = SPIFFS.openDir(dirname);
  while (root.next ()) {
    // gather the data about the file we need
    File filee = root.openFile("r");
    String fullName = filee.fullName();
    String size = String(filee.size());
    filee.close();
#endif	
  // *********  common for ESP32 and ESP8266
  // while(file-variable){
  //      
    if (! fullName.startsWith("/")) {
      // files in root are assumed to not have a leading '/'
      fullName = "/" + fullName;
    }
	// pseudo dir-path is a "file-name" with ending '/'
	// this is a pseudo representation for the DevFsUploadSPIFFS design
    boolean isFileAsPseudoDir = fullName.endsWith("/");

    if (listFiles) {
        content += "<div>" + fullName + " " + size + "</div>";
        // client.println(str);
      continue;
    }
    // build directory list from file name, but only if
    // looking at directory list
    //
    // file name has a last '/',  implying before the slash is a dir-path
    int lastSlashIdx = fullName.lastIndexOf("/");
    String dirNam = fullName.substring(0, lastSlashIdx + 1);

    boolean dirFnd = false;
    for (int i = 0; i < dirArrNextIdx; i++) {
      if (dirNam == dirArr[i]) {
        dirFnd = true;

		// remove any pseudo-dir-file items if there is a real file
		// in the SPIFFS structure
        if (dirPseudoArr[i] || isFileAsPseudoDir) {
          // cleanup any pseudo-dir-file
          dirPseudoArr[i] = false;
          SPIFFS.remove(dirNam);
        }
        break;
      }
    }
    // if the directory is not found in the list, then its unique and
    // needs to be recorded
    if (!dirFnd) {
      // record the information for the directory name and if its
      // a pseudo directory (that is SPIFFS does not support directories
      // so file-name ending in '/' is a file that represents a directory)
      dirArr[dirArrNextIdx] = dirNam;
      dirPseudoArr[dirArrNextIdx] = isFileAsPseudoDir;
      dirArrNextIdx++;

      // // detect pseudo directories and present differently to the
      // // client side for indicating as such
      // if(fullName.endsWith("/")){
      // dirNam += "/";
      // }
      // list directory
      content += "<div>" + dirNam  + " " + (isFileAsPseudoDir ?  "1" : "0")
      + "</div>";

      // client.println(str);
    } // end while
  }
  return content;
}


//SPIFFS
File DevFsUploadESP::openFile4WriteTargeted(String fn){
	return SPIFFS.open(fn, "w");
}

//SPIFFS 
File DevFsUploadESP::openFile4ReadTargeted(String fn){
	return SPIFFS.open(fn, "r");
}

//SPIFFS
void DevFsUploadESP::deleteFileTargeted(String fn){
    if (SPIFFS.exists(fn)) {
      SPIFFS.remove(fn);
    }
}

// SPIFFS
void DevFsUploadESP::mkDirFSTargeted(String nuDirFullPath){
	// for SPIFFS create a pseudo-directory by making a file
	// that looks like a directory. This will be processed upstream
	// on the browser when displaying List or list-dir.
   SPIFFS.mkdir(nuDirFullPath);
   File mkdirFile = SPIFFS.open(nuDirFullPath + "/", "w");

   if (mkdirFile) {
      mkdirFile.close();
   }
}

// SPIFFS
void DevFsUploadESP::rmDirFSTargeted(String dirname) {
  // SPIFFS provides rmdir, but it needs the directory to be
  // empty
  
#ifdef ESP32
  // dirname comes with ending '/' character, which does not work
  // on ESP32 SPIFFS
  //   
  // '/a/b/c/d/' will not open as a dir, whereas '/a/b/c/d' will
  // this is because SPIFFS elements are all files
  //
  // note, while '/' works and '' does not
  //
  // process real directories
  String dirNam = dirname;
  int len = dirNam.length();
  if(len > 1){
      dirNam = dirNam.substring(0, len-1);
  }
  // begin the process
  File root = SPIFFS.open(dirNam);
  File filee;
  
  while (filee = root.openNextFile()) {
      if(filee.isDirectory()){
          DevFsUploadESP::rmDirFSTargeted(filee.name());
      }else{
          // assume is a file
           SPIFFS.remove(filee.name());
      }
  }
  // clean up the top directory
  root = SPIFFS.open(dirname);
  if(!root.isDirectory()){
     // is a pseudo-dir so remove it as a file
     SPIFFS.remove(dirname);
  }else{
     SPIFFS.rmdir(dirNam);
  }
#else
  // ESP8266

  // open a directory for processing and then process through the
  // contents to get files or directory information
  
  // lastslash is not a problem
  //
  Dir root = SPIFFS.openDir(dirname);
  while (root.next ()) {
    if (root.isFile()) {

      // gather the data about the file we need
      File filee = root.openFile("r");
      String fullName = filee.fullName();
      filee.close();

      SPIFFS.remove(fullName);
    } else if (root.isDirectory()) {
      // unfortunately SPIFFS does not provide a full-name/full-path
      // method for directories, so the following code emulates
      // full-path construction
      String dirNam = String(dirname);
      dirNam += root.fileName();
      
      // NB: code above is 16 bytes less progmem than
      //     String dirNam = String(dirname) + root.fileName();
      
      // process the sub-directory
      DevFsUploadESP::rmDirFSTargeted(dirNam);

      // SPIFFS automatically deletes the directory when it becomes
      // empty, so no explicit action is required.
    }
  }
  // clean up the top directory
  if(!root.isDirectory()){
     // is a pseudo-dir so remove it as a file
     SPIFFS.remove(dirname);
  }else{
     SPIFFS.rmdir(dirname);
  }
#endif
}

