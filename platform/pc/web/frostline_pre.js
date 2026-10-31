// Keeps the player's record in the browser (IndexedDB) for the web build.
//
// The game writes /save/frostline_record.bin like any file. Before it starts,
// the folder is filled from IndexedDB; afterwards it is written back every
// few seconds and whenever the page is hidden or closed.
//
// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
  FS.mkdir('/save');
  FS.mount(IDBFS, {}, '/save');
  // Busy from the first read until it lands: writing back an empty folder
  // over the saved one would lose the record.
  var syncing = true;
  function persist() {
    if (syncing) return;
    syncing = true;
    FS.syncfs(false, function (error) {
      syncing = false;
      if (error) console.warn('frostline: could not save the record', error);
    });
  }

  addRunDependency('frostline-save');
  FS.syncfs(true, function (error) {
    if (error) console.warn('frostline: could not read the saved record', error);
    syncing = false;
    removeRunDependency('frostline-save');
  });

  setInterval(persist, 3000);
  document.addEventListener('visibilitychange', function () {
    if (document.visibilityState === 'hidden') persist();
  });
  window.addEventListener('pagehide', persist);
});
