# DevFsUploadESP
 ESP32/8266 file-system upload development tool for SPIFFS or LittleFS
 
 A Arduino set of libraries that provides a browser-UI to manage file-system components on the ESP.
 
 A user needs to install the libraries (found in the **lib** folder):<br/>
     **DevFsUploadCommon**, **DevFsUploadSPIFFS** and **DevFsUploadLittleFS**
     
 See the doc/devfsuploadesp_docs files (download and view .html will a browser).
     
 The capabilities provided are:<br/>
 files: list, delete, view<br/>
 dirs: list, select (for upload ESP-target-directory) make-sub-directory, remove-directory
 
 upload browse: reset, upload, browse  (all for files ro directory management
 
 ESP-target-directory : 'manual' or 'select dir' setting capability<br/>
 (upload target)

NOTE/WARNING: there is no checking for free space on the ESP when uploading files.

Initial page view

![Alt text](./initialpage.png)

List files and List Dir(s) view

![Alt text](./listfiles.png)  ![Alt text](./listdirl.png)
