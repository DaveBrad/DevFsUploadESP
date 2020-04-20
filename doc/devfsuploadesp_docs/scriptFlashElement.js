/**
 * Copyright (c) 2013-2020 dbradley. All rights reserved.
 * 
 * This is an original piece of work for a specific use. There is no warranty as
 * to fitness of the script herein for another entity's use (persons, companies).
 * 
 * The script file may be used freely as an executable as long as it remains
 * in its original form, with the copyright intact. Original form means no
 * alterations of any kind (extraction or directives of functions from into
 * another script file).
 */
var flashColor = "orange";

/**
 * Flash a href link elements (hyper-link point) and scroll-to to place it in
 * view.
 * 
 * @param hrefThis
 *            the href element the call on-click is used in
 */
function flashHrefLinkElement(hrefThis) {
  flashAndScrollFromHref(hrefThis.href);



  /**
   * Flash the provided document element (assumes element has color and is
   * accessible) by changing its color.
   * 
   * This should be used slightly and used for hi-lighting the change in
   * Javascript scroll into view of headings. (If the heading is at the bottom of
   * a HTML page it may appear within the page is not scrolled to the top of the
   * view. The flash is meant to help a user see the appropriate heading.)
   */
  function flashElement(elementToFlash) {
    // a counter that on the first cycle will be considered
    // a change from initial flash to the nth flash
    this.firstCycleDone = 0;

    // initial flash length provides enough time for a user to notice
    // the color before the flashing starts
    this.initialFlash = 1000;
    // the nth flash timer
    this.nthFlash = 350;

    // the milliseconds for the flash timer (0-200 or 350 +
    // as 300 is a bad flash rate for certain people
    this.timeIntervalMs = this.initialFlash;

    // length of the flashing time
    this.timeLengthMs = 3000;
    // the start of the flash
    this.flashCountTime = 0;

    // setInterval ID object
    this.nIntervalId = null;

    // the element to flash record for this to process on
    this.ele_ToFlash = elementToFlash; // document element to flash
    this.origColor = elementToFlash.style.backgroundColor; // original color to

    /**
     * Perform the flash by toggling the color and stop when the length to run
     * finishes.
     */
    this.flashFunction = function () {
      // alternate the color for the flash cycle
      if (this.ele_ToFlash.style.backgroundColor === flashColor) {
        // revert to original color
        this.ele_ToFlash.style.backgroundColor = this.origColor;
      } else {
        this.ele_ToFlash.style.backgroundColor = flashColor;
      }
      // count by time until it is exceeded
      if (this.flashCountTime > this.timeLengthMs) {
        this.flashStopOnElement();
      } else {
        this.flashCountTime += this.timeIntervalMs;
      }
      // change the interval after the 1st/initial cycle
      //
      if (this.firstCycleDone === 1) {
        // set the newer interval timer
        this.timeIntervalMs = this.nthFlash;

        // clear the old timer interval and start a newer one
        clearInterval(this.nIntervalId);
        this.startFlash();
      }
      // do a count of cycles as the first count needs to change the
      // timer from the initial flash timer to the flash interval
      this.firstCycleDone++;

      return this;
    };

    this.startFlash = function () {
      // in newer browsers IE 11, FF 31 the bind has been implemented, but not
      // on all
      //
      // this.nIntervalId = setInterval(this.flashFunction.bind(this) , 500);

      // placing 'this' into 'var self' creates a nested closure (don't fully
      // understand the mechanism but this works on newer browsers and some
      // older browser QtWeb 3.8.5
      var self = this;

      // setInterval in a closure
      this.nIntervalId = setInterval(function () {
        return self.flashFunction();
      }, this.timeIntervalMs);
    };

    /**
     * Stop the flash of the color on the element.
     */
    this.flashStopOnElement = function () {
      clearInterval(this.nIntervalId);
      this.ele_ToFlash.style.backgroundColor = this.origColor;
      this.nIntervalId = null;
    };
  }

  /**
   * Flash the element that is known by the idName. Present an alert if the
   * element-id is not found
   * 
   * @param idName
   *            string of the element-id to flash
   */
  function flashElementId(idName) {
    var flash_element = document.getElementById(idName);

    if (flash_element === null) {
      // the alert is no good as it provides no information
      // so just return as no flash is possible on this page
      // alert("<a id=\"...\"..</a> not found: " + idName);

      return;
    }
    // scroll to the element and place it in view
    flash_element.scrollIntoView(true);

    // the element x-scroll needs to be adjusted to the beginning
    // of the line (edge of page)
    var yCoord = window.pageYOffset || document.documentElement.scrollTop;

    window.scrollTo(0, yCoord);

    new flashElement(flash_element).startFlash();
  }

  /**
   * Flash and scroll to the hyper-link point for a href link string.
   * 
   * @param hrefString
   *            the string that contains the element-id string (#id-name)
   */
  function flashAndScrollFromHref(hrefString) {
    /* there are two ways to do the flash */

    var hrefPoundIdx = hrefString.indexOf("#");

    if (hrefPoundIdx === -1) {
      // is a top of the page href action
      return;
    }
    // is a section within the document and it would be nice to
    // flash the section so people know what is being selected
    var idOfElement2Flash = hrefString.substring(hrefPoundIdx + 1);

    flashElementId(idOfElement2Flash);

    return;
  }

  /**
   * On load do a flash and scroll-to action for the href item being linked to
   * within the page
   */
  function onLoadFlashAndScroll() {
    flashAndScrollFromHref(window.location.href);
  }
}