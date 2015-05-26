/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

var MockFilePicker = SpecialPowers.MockFilePicker;
MockFilePicker.init(window);

/**
 * Test for bug 471962 <https://bugzilla.mozilla.org/show_bug.cgi?id=471962>:
 * When saving an inner frame as file only, the POST data of the outer page is
 * sent to the address of the inner page.
 *
 * Test for bug 485196 <https://bugzilla.mozilla.org/show_bug.cgi?id=485196>:
 * Web page generated by POST is retried as GET when Save Frame As used, and the
 * page is no longer in the cache.
 */
function test() {
  waitForExplicitFinish();

  gBrowser.loadURI("http://mochi.test:8888/browser/toolkit/content/tests/browser/data/post_form_outer.sjs");

  registerCleanupFunction(function () {
    gBrowser.addTab();
    gBrowser.removeCurrentTab();
  });

  gBrowser.addEventListener("pageshow", function pageShown(event) {
    if (event.target.location == "about:blank")
      return;
    gBrowser.removeEventListener("pageshow", pageShown);

    // Submit the form in the outer page, then wait for both the outer
    // document and the inner frame to be loaded again.
    gBrowser.addEventListener("DOMContentLoaded", handleOuterSubmit);
    gBrowser.contentDocument.getElementById("postForm").submit();
  });

  var framesLoaded = 0;
  var innerFrame;

  function handleOuterSubmit() {
    if (++framesLoaded < 2)
      return;

    gBrowser.removeEventListener("DOMContentLoaded", handleOuterSubmit);

    innerFrame = gBrowser.contentDocument.getElementById("innerFrame");

    // Submit the form in the inner page.
    gBrowser.addEventListener("DOMContentLoaded", handleInnerSubmit);
    innerFrame.contentDocument.getElementById("postForm").submit();
  }

  function handleInnerSubmit() {
    gBrowser.removeEventListener("DOMContentLoaded", handleInnerSubmit);

    // Create the folder the page will be saved into.
    var destDir = createTemporarySaveDirectory();
    var file = destDir.clone();
    file.append("no_default_file_name");
    MockFilePicker.returnFiles = [file];
    MockFilePicker.showCallback = function(fp) {
      MockFilePicker.filterIndex = 1; // kSaveAsType_URL
    };

    mockTransferCallback = onTransferComplete;
    mockTransferRegisterer.register();

    registerCleanupFunction(function () {
      mockTransferRegisterer.unregister();
      MockFilePicker.cleanup();
      destDir.remove(true);
    });

    var docToSave = innerFrame.contentDocument;
    // We call internalSave instead of saveDocument to bypass the history
    // cache.
    internalSave(docToSave.location.href, docToSave, null, null,
                 docToSave.contentType, false, null, null,
                 docToSave.referrer ? makeURI(docToSave.referrer) : null,
                 docToSave, false, null);
  }

  function onTransferComplete(downloadSuccess) {
    ok(downloadSuccess, "The inner frame should have been downloaded successfully");

    // Read the entire saved file.
    var file = MockFilePicker.returnFiles[0];
    var fileContents = readShortFile(file);

    // Check if outer POST data is found (bug 471962).
    is(fileContents.indexOf("inputfield=outer"), -1,
       "The saved inner frame does not contain outer POST data");

    // Check if inner POST data is found (bug 485196).
    isnot(fileContents.indexOf("inputfield=inner"), -1,
          "The saved inner frame was generated using the correct POST data");

    finish();
  }
}

Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://mochitests/content/browser/toolkit/content/tests/browser/common/mockTransfer.js",
                 this);

function createTemporarySaveDirectory() {
  var saveDir = Cc["@mozilla.org/file/directory_service;1"]
                  .getService(Ci.nsIProperties)
                  .get("TmpD", Ci.nsIFile);
  saveDir.append("testsavedir");
  if (!saveDir.exists())
    saveDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  return saveDir;
}

/**
 * Reads the contents of the provided short file (up to 1 MiB).
 *
 * @param aFile
 *        nsIFile object pointing to the file to be read.
 *
 * @return
 *        String containing the raw octets read from the file.
 */
function readShortFile(aFile) {
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);
  inputStream.init(aFile, -1, 0, 0);
  try {
    var scrInputStream = Cc["@mozilla.org/scriptableinputstream;1"]
                           .createInstance(Ci.nsIScriptableInputStream);
    scrInputStream.init(inputStream);
    try {
      // Assume that the file is much shorter than 1 MiB.
      return scrInputStream.read(1048576);
    }
    finally {
      // Close the scriptable stream after reading, even if the operation
      // failed.
      scrInputStream.close();
    }
  }
  finally {
    // Close the stream after reading, if it is still open, even if the read
    // operation failed.
    inputStream.close();
  }
}
