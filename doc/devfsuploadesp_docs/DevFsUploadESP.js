/* Copyright (c) 2020 dbradley */

function myloaded() {
  tabsBuilder();
  doDuplicates();
  allHeadings();
  allATags();
  setATagText();
}

/**
 'chgSeeHide' change see-class-state to hide-class-state for all
 elements with the provided classes.
 
 Change the see elements to hide, then invoking function will set
 the appropriate element to be see-state.
 */
function chgSeeHide(seeClz, hideClz) {
  var elesArr = document.getElementsByClassName(seeClz);

  document.gete
  var i;

  // for (var i = 0; i < elesArr.length; i++) {
  // var x = elesArr[i];
  //
  // changing the classList and as such the elesArr will be altered too
  while (elesArr.length > 0) {
    var x = elesArr[0];
    x.classList.add(hideClz);
    x.classList.remove(seeClz);
  }
}

/**
 'tabDisp' processes the "content_ div" elements for hide or see class
 states, along with the "tabs" hide or see class states     
 */
function tabDisp(ele) {
  // the content_ div's
  chgSeeHide("contentsee", "contenthide");
  // the "tabs"
  chgSeeHide("tabssee", "tabshide");

  ele.classList.remove("tabshide");
  ele.classList.add("tabssee");

  var contentId = "content_" + ele.getAttribute("id");
  var x = document.getElementById(contentId);
  x.classList.add("contentsee");
  x.classList.remove("contenthide");
  
  // change tabs ensure the top of the page is scrolled-to
  x.parentNode.scrollTo(0, 0);
}

/**
 'ATagOnClick' deals with ATag/links that href into 
 parent-hidden elements within the tabs structure. The
 tab will be opened and the link completed.
 */
function aTagOnClick(ele) {

  var hrefVal = ele.getAttribute("href");
  if (hrefVal.startsWith("#")) {
    // internal to the tab architecture of the page, where some
    // elements may be hidden under the tab <div ...> condition

    var hrefAsId = hrefVal.substring(1); // remove '#'
    var hrefTooEle = document.getElementById(hrefAsId);

    var hrefIsVisible = hrefTooEle.offsetHeight;

    if (!hrefIsVisible) {
      // assume the item is in a hidden tab <div>, so need
      // to make it visible (swap to the tab and then href to it)

      var allTabDivArr = document.getElementsByClassName("contenthide");

      for (var i = 0; i < allTabDivArr.length; i++) {
        var divEle = allTabDivArr[i];

        var isChildNodeOf = divEle.contains(hrefTooEle);

        if (isChildNodeOf) {
          // tabEle is the div that needs to be set visible
          // 1) find the associated TAB-button
          //.......................>------<
          //      <div id="content_sketchid" class="contenthide">
          //..................>------<
          //      <button id="sketchid"  class="tabsbtt tabshide"
          //
          // 2) click the TAB-button
          // 3) do an internal HREF on it (done if return true
          //    by HTML control in browser FF, Chrome, )

          var devId = divEle.getAttribute("id");
          var idxUnder = devId.indexOf("_");
          var tabButtonId = devId.substring(idxUnder + 1);

          var hrefThis = document.getElementById(tabButtonId);
          document.getElementById(tabButtonId).click();
        }
      }
    }
    flashHrefLinkElement(ele);
  }
  return true;
}

/**
 'allATags' changes all 'a' tags to use an onclick action 
 of 'ATagOnClick'action.
 */
function allATags() {
  // add a onclick to all 'a' tags so as to control the processing
  // of href links in relationship to the topic tabs architecture
  //
  var aTagsArr = document.getElementsByTagName("a");

  for (var i = 0; i < aTagsArr.length; i++) {
    var aTagEle = aTagsArr[i];

    aTagEle.setAttribute("onclick", "aTagOnClick(this);");
  }
}

/**
 'allHeadings' will get all the headings and number them in order
 reguard-less of parent-hidden state.
 
 The headings are in order so the tabs heading, are in sequest.
 
 
 Heading numbering can be done uses CSS counter, counter-reset and 
 counter-increment, the algorithm will not number elements that
 are parent-hidden. FF also does counter-set, but this is not
 supported by Chrome, Edge, IE,......
 */
function allHeadings() {
  var idLinkCnt = 0;

  var h1Cnt = 0;
  var h2Cnt, h3Cnt, h4Cnt, h5Cnt, h6Cnt;

  // assume the headings start with 'h1'
  var allHeadingsArr = document.querySelectorAll("h1, h2, h3, h4, h5, h6");

  for (var i = 0; i < allHeadingsArr.length; i++) {
    var hdEle = allHeadingsArr[i];

    // there are means to turn-off heading numbering and inclusion
    // into the table-of-contents (TOC) per heading element
    var noNum = hdEle.classList.contains("nohdgnum");
    var noToc = hdEle.classList.contains("nohdgtoc");

    var hdgLevel = 0;

    // ADD the NUMBERING is it applies
    if (!noNum) {
      var hdTag = hdEle.tagName.toLowerCase();
      var hdrText = "";

      if (hdTag === "h1") {
        hdgLevel = 1;
        h1Cnt++;
        h2Cnt = h3Cnt = h4Cnt = h5Cnt = h6Cnt = 0;
        hdrText = h1Cnt + "";

      } else if (hdTag === "h2") {
        hdgLevel = 2;
        h2Cnt++;
        h3Cnt = h4Cnt = h5Cnt = h6Cnt = 0;

        hdrText = h1Cnt + "." + h2Cnt;

      } else if (hdTag === "h3") {
        hdgLevel = 3;
        h3Cnt++;
        h4Cnt = h5Cnt = h6Cnt = 0;
        hdrText = h1Cnt + ".";
        hdrText += h2Cnt + "." + h3Cnt;

      } else if (hdTag === "h4") {
        hdgLevel = 4;
        h4Cnt++;
        hdrText = h1Cnt + ".";
        hdrText += h2Cnt + ".";
        hdrText += h3Cnt + "." + h4Cnt;

      } else if (hdTag === "h5") {
        hdgLevel = 5;
        h5Cnt++;
        hdrText = h1Cnt + ".";
        hdrText += h2Cnt + ".";
        hdrText += h3Cnt + ".";
        hdrText += h4Cnt + "." + h5Cnt;

      } else if (hdTag === "h6") {
        hdgLevel = 6;
        h6Cnt++;
        hdrText = h1Cnt + ".";
        hdrText += h2Cnt + ".";
        hdrText += h3Cnt + ".";
        hdrText += h4Cnt + ".";
        hdrText += h5Cnt + "." + h6Cnt;
      }
    }
    // hdrText if filled in will be prefixed
    if (hdrText !== "") {
      // innerHTML...........VVVVVVVVVVVVVVVVVVVVVVVVVVV
      //
      // "<h1 id=\"abouth1\">About <i>DevFsUploadESP</i></h1>"
      //.. above is outerHTML
      hdEle.innerHTML = hdrText + ". " + hdEle.innerHTML;
    }
    // ADD table-of-contents if it applies for this heading element
    if (!noToc) {
      // need to include this item into the table of contents
      //
      // TOC will create a link system automatically, so ignore the word
      var tocSectEle = document.getElementById("tocsect");

      if (tocSectEle !== null) {
        // document has a table-of-contents sections

        // does the heading have an 'id' to link too
        var hdgId = hdEle.id;

        if (hdgId === null || hdgId === "") {
          var makeUniqueId = true;
          while (makeUniqueId) {
            // need to create an ID
            var createdID = "_toclink" + idLinkCnt;

            // make sure we do not have one of these
            var alreadyId = document.getElementById(createdID);
            if (alreadyId === null) {
              makeUniqueId = false;
              hdgId = createdID;

            } else {
              idLinkCnt++;
            }
          }
        }
        hdEle.id = hdgId;
      }

      // now have an ID for sure, add to the TOC section
      //
      // just need to add this heading item to the list (with
      // ATag href appropriate code) with indenting
      var tocParaIndent = hdgLevel * 6;
      tocParaIndent = "margin-left: " + tocParaIndent + "px;";

      var nuPEle = document.createElement("P");
      nuPEle.setAttribute("style", tocParaIndent);


      var hrefS = "#" + hdgId;
      var nuAEle = document.createElement("A");
      nuAEle.setAttribute("href", hrefS);

      // assumption heading are simple HTML
      nuAEle.innerHTML = hdgLevel === 1 ? "<br>" : "";
      nuAEle.innerHTML += hdEle.innerHTML;

      nuPEle.appendChild(nuAEle);

      tocSectEle.appendChild(nuPEle);
    }
  }
}

/**
 build the tabs display for each item of content below, with the first item
 being set see-able
 */
function tabsBuilder() {
  var csArr = document.getElementsByClassName("contentsection");
  var tabsContr = document.getElementById("tabscontainer");

  // convert into tabs making the first one visible content
  var firstNotDone = true;

  // an element will be removed from the DOM in the following
  // loop, it happens to be in csArr and as such will pop-extract the item
  // from the csArr too (so a for loop doesn't work)
  while (csArr.length > 0) {
    // contentsection
    // <section class="contentsect">User Guild</section>
    var csEle = csArr[0];

    var contDivId = csEle.parentNode.id;
    // id should be 'content_xxxxxxx'
    var idx = contDivId.indexOf("_");
    var sectNameid = contDivId.substring(idx + 1);

    // <div id="tabscontainer"></div>
    // <button id="ugid"  class="tabsbtt tabssee"
    //        onclick="tabDisp(this);" tabindex="0">User Guide</button>
    //
    var nuButt = document.createElement("button");
    nuButt.setAttribute("id", sectNameid);
    nuButt.setAttribute("class", "tabsbtt");
    nuButt.setAttribute("tabindex", "0");
    nuButt.setAttribute("onclick", "tabDisp(this);");

    // the first item in the list is made see-able
    nuButt.innerHTML = csEle.innerHTML;

    tabsContr.appendChild(nuButt);

    var csEleParent = csEle.parentNode;

    // see the first one, all others assumed to be hide
    var setSeeHide = "tabshide";
    var setSeeHide4Content = "contenthide";

    if (firstNotDone) {
      setSeeHide = "tabssee";
      setSeeHide4Content = "contentsee";

      firstNotDone = false;
    }
    // set the see-or-hide via the class-list
    nuButt.classList.add(setSeeHide);
    csEleParent.classList.add(setSeeHide4Content);

    csEleParent.removeChild(csEle);
  }
  // copy right on the tabs
  //
  // <div class="tabcpyrgt"><div data-duplicate="copyright"></div></div>
  //
  var divCpyRgt1 = document.createElement("div");
  divCpyRgt1.classList.add("tabcpyrgt");
  
  var divCpyRgt2 = document.createElement("div");
  divCpyRgt2.setAttribute("data-duplicate","copyright");
  
  
  divCpyRgt1.appendChild(divCpyRgt2);
  tabsContr.appendChild(divCpyRgt1);
}

/**
 Set the ATag text on ATags that have the class setATagText
 with the href-innerHTML as the origin text for display. 
 */
function setATagText() {
  var aTagsArr = document.getElementsByClassName("setatagtext");

  for (var i = 0; i < aTagsArr.length; i++) {
    var aTagEle = aTagsArr[i];

    var href = aTagEle.getAttribute("href");
    if (href === null)
      continue;

    if (!href.startsWith("#"))
      continue;

    // has a internal link which can be text found
    href = href.substring(1);

    var originHref = document.getElementById(href);

    if (originHref !== null) {
      aTagEle.innerHTML = originHref.innerHTML;
    }
  }
}

/**
 Change page to be print friendly.
 
 All the content_xxxxx DIVs are made see-able, the 'tabs' buttons are
 removed so the document is continuous.
 */
function printFriendly() {
  // remove the tabs section
  var tabsEle = document.getElementsByClassName("divtabszone")[0];
  tabsEle.parentNode.removeChild(tabsEle);

  // remove the TOC contentId
  var tocEle = document.getElementById("content_tocid");
  tocEle.parentNode.removeChild(tocEle);

  // remove the class that controls the display of the content within
  // a page and provides scrolling.
  var divContentAll = document.getElementsByClassName("divcontentcontainer");
  divContentAll[0].classList.remove("divcontentcontainer");

  // make all the hidden content see-able
  chgSeeHide("contenthide", "contentsee");
}

/**
 'doDuplicates' will create duplicate/clone of other HTML element (such as, tables that
 need to be repeated in different sections for clarity).
 
 Only works within thw same document.
 
 A DIV tag is the best (but should work on other tags) to duplicate/clone. The are two elements
 involved in the duplication:
 1) original -- a element with an ID attribute (and childern nodes....)
 2) duplicate --  a like element but with a 'data-duplicate=' attribute
 eg.
 <DIV data-duplicate='originals-id'> </div>
 
 All the data-duplicate elements are found in the DOM. Then the "original-id"
 extracted and DOM-element with the ID is deep-cloned (duplicated). [Some screening is done
 to ensure that elements are valid.]
 
 The clone is post-processed and all ID's changed to be  'originalID + "__" + number' so as to 
 ensure a unique ID for the cloned-element(s).
 
 The 'data-duplicate=' is need replaced with the cloned-duplicate element.
 
 
 */
function doDuplicates() {
  // <div data-duplicate="suppconfig">
  //
  var allDuplArr = document.querySelectorAll('[data-duplicate]');

  for (var i = 0; i < allDuplArr.length; i++) {
    // the data attribute identifies the ID of the original
    var duplEle = allDuplArr[i];

    var originId = duplEle.getAttribute("data-duplicate");
    if (originId === null)
      continue;

    var originEle = document.getElementById(originId);

    // if no origin nothing can be done
    if (originEle === null)
      continue;

    // now do the copy 
    var cloneEle = originEle.cloneNode(true);

    // need to change the IDs to be unique for the clone, basically
    // take the ID and append '__n' to the end of original-ID
    // (its hoped to be a different enough pattern to be unique)
    // (someone who does '__n' for IDs is likely to be rare-rare....)
    travseId(cloneEle);

    // need to retain the original ID for the duplicate/clone, as the
    // travse has will has set it as the original-ID modified
    var duplOrgEleID = duplEle.id;

    if (duplOrgEleID !== "") {
      cloneEle.setAttribute("id", duplOrgEleID);
    }
    cloneEle.setAttribute("data-dupl-of", originId);
    duplEle.replaceWith(cloneEle);
  }


  /**
   travse a node's childern and upate the ID attribute if it is set.
   */
  function travseId(ele) {

    var elesChildArr = ele.childNodes;

    for (var i = 0; i < elesChildArr.length; i++) {
      var childEle = elesChildArr[i];

      if (childEle.nodeType === 3)
        continue;  // #text ignore

      if (childEle.childNodes.length > 0) {
        travseId(childEle)
      }
    }
    uniqueId(ele);
  }

  /**
   Change to a 'uniqueId' the current-element being processed if it has an ID
   attribute. [Ensuring .]
   
   A cloned-element will have the same ID as the 'originalId' initially, this needs to be changed
   and to a unique ID. This is done by suffixing "__n" where n is a number counter to the originslID.
   The 
   */
  function uniqueId(currEle) {
    var cloneId = currEle.getAttribute("id");

    if (cloneId === null)
      return;
    // have an element with an id
    var cloneNum = 1;
    var uniqueID = cloneId + "__" + cloneNum;

    var testUniqueID = document.getElementById(uniqueID);

    while (testUniqueID !== null) {
      cloneNum++;
      uniqueID = cloneId + "__" + cloneNum;
      testUniqueID = document.getElementById(uniqueID);
    }
    // have a unique ID for the clone, need to change it to this
    currEle.setAttribute("id", uniqueID);
  }
}