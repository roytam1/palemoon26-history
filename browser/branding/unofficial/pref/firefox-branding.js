// ****************** App/Update/General ******************

pref("startup.homepage_override_url","http://www.palemoon.org/releasenotes.shtml");
pref("startup.homepage_welcome_url","http://www.palemoon.org/firstrun.shtml");
// Interval: Time between checks for a new version (in seconds) -- 2 days for Pale Moon
pref("app.update.interval", 172800);
pref("app.update.auto", false);
pref("app.update.enabled", false);
// URL for update checks, re-enabled on palemoon.org (369)
pref("app.update.url", "http://www.palemoon.org/update/%VERSION%/%BUILD_TARGET%/update.xml");
pref("app.update.promptWaitTime", 86400); 
// The time interval between the downloading of mar file chunks in the
// background (in seconds)
pref("app.update.download.backgroundInterval", 600);
// URL user can browse to manually if for some reason all update installation
// attempts fail.
pref("app.update.url.manual", "http://www.palemoon.org/");
// A default value for the "More information about this update" link
// supplied in the "An update is available" page of the update wizard. 
pref("app.update.url.details", "http://www.palemoon.org/releasenotes.shtml");
// Additional Update fixes - no SSL damnit, I don't have a cert (4.0)
pref("app.update.cert.checkAttributes", false);
pref("app.update.cert.requireBuiltIn", false);
// Fix useragent for UA sniffing websites
pref("general.useragent.compatMode.firefox", true);
// Make sure we shortcut out of a11y to save walking unnecessary code
pref("accessibility.force_disabled", 1);

// ****************** Release notes and vendor URLs ******************

pref("app.releaseNotesURL", "http://www.palemoon.org/releasenotes.shtml");
pref("app.vendorURL", "http://www.palemoon.org/");
pref("app.support.baseURL", "http://www.palemoon.org/support/");
//Add-on window fixes
pref("extensions.getAddons.browseAddons", "https://addons.mozilla.org/%LOCALE%/firefox");
pref("extensions.getAddons.maxResults", 10);
pref("extensions.getAddons.recommended.browseURL", "https://addons.palemoon.org/integration/addon-manager/external/recommended");
pref("extensions.getAddons.recommended.url", "https://addons.palemoon.org/integration/addon-manager/internal/recommended?locale=%LOCALE%&os=%OS%");
pref("extensions.getAddons.search.browseURL", "https://addons.palemoon.org/integration/addon-manager/external/search?q=%TERMS%");
pref("extensions.getAddons.search.url", "https://addons.palemoon.org/integration/addon-manager/internal/search?q=%TERMS%&locale=%LOCALE%&os=%OS%&version=%VERSION%");
pref("extensions.getMoreThemesURL", "https://addons.palemoon.org/integration/addon-manager/external/themes");
pref("extensions.blocklist.url", "https://addons.mozilla.org/blocklist/3/firefox/%APP_VERSION%/%PRODUCT%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/");
pref("extensions.blocklist.itemURL", "https://addons.mozilla.org/%LOCALE%/firefox/blocked/%blockID%");
pref("extensions.webservice.discoverURL","http://addons.palemoon.org/integration/addon-manager/internal/discover/");
pref("extensions.getAddons.cache.enabled", false);
pref("extensions.getAddons.get.url","https://addons.palemoon.org/integration/addon-manager/internal/get?addonguid=%IDS%&os=%OS%&version=%VERSION%");
pref("extensions.getAddons.getWithPerformance.url","https://addons.palemoon.org/integration/addon-manager/internal/get?addonguid=%IDS%&os=%OS%&version=%VERSION%");
//Add-on updates: hard-code base Firefox version number.
pref("extensions.update.background.url","https://addons.palemoon.org/integration/addon-manager/internal/update?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%&compatMode=%COMPATIBILITY_MODE%");
pref("extensions.update.url","https://addons.palemoon.org/integration/addon-manager/internal/update?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&status=%ITEM_STATUS%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%&locale=%APP_LOCALE%&currentAppVersion=%CURRENT_APP_VERSION%&updateType=%UPDATE_TYPE%&compatMode=%COMPATIBILITY_MODE%");
//Search engine fixes
pref("browser.search.searchEnginesURL", "https://addons.mozilla.org/%LOCALE%/firefox/search-engines/");
//Dictionary URL
pref("browser.dictionaries.download.url", "https://addons.mozilla.org/%LOCALE%/firefox/dictionaries/");
//Geolocation info URL
pref("browser.geolocation.warning.infoURL", "http://www.mozilla.com/%LOCALE%/firefox/geolocation/");
//add-on/plugin blocklist -> Palemoon.org
pref("extensions.blocklist.url","http://blocklist.palemoon.org/%VERSION%/blocklist.xml");
pref("extensions.blocklist.itemURL", "http://blocklist.palemoon.org/info/?id=%blockID%");

// ****************** domain-specific UAs ******************

// Required for domains that have proven unresponsive to requests from users
pref("general.useragent.override.live.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.0) Gecko/20100101 Firefox/38.0 (Pale Moon)");
pref("general.useragent.override.outlook.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.0) Gecko/20100101 Firefox/38.0 (Pale Moon)");
pref("general.useragent.override.web.de","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.0) Gecko/20100101 Firefox/38.0 (Pale Moon)");
pref("general.useragent.override.aol.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.0) Gecko/20100101 Firefox/38.0 (Pale Moon)");
pref("general.useragent.override.mail.google.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.9) Gecko/20100101 Goanna/2.0 Firefox/38.9 PaleMoon/26.0");
pref("general.useragent.override.google.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:31.9) Gecko/20100101 Goanna/2.0 Firefox/31.9 PaleMoon/26.0");
pref("general.useragent.override.googlevideos.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:31.9) Gecko/20100101 Goanna/2.0 Firefox/31.9 PaleMoon/26.0");
pref("general.useragent.override.fonts.googleapis.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:31.9) Gecko/20100101 Goanna/2.0 Firefox/31.9 PaleMoon/26.0");
pref("general.useragent.override.gstatic.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:31.9) Gecko/20100101 Goanna/2.0 Firefox/31.9 PaleMoon/26.0");
pref("general.useragent.override.youtube.com","Mozilla/5.0 (Windows NT 5.1; rv:42.0) Gecko/20100101 Firefox/42.0 PaleMoon/26.0");
pref("general.useragent.override.netflix.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:26.0) Gecko/20100101 Firefox/31.9");
pref("general.useragent.override.calendar.yahoo.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:45.0) Gecko/20100101 Firefox/45.0 (Pale Moon)");

// UA-Sniffing domains below are pending responses from their operators - temp workaround
pref("general.useragent.override.facebook.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.9) Gecko/20100101 Goanna/2.0 Firefox/38.9 PaleMoon/26.0");
pref("general.useragent.override.fbcdn.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.9) Gecko/20100101 Goanna/2.0 Firefox/38.9 PaleMoon/26.0");
pref("general.useragent.override.fbcdn.net","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.9) Gecko/20100101 Goanna/2.0 Firefox/38.9 PaleMoon/26.0");
pref("general.useragent.override.chase.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:31.9) Gecko/20100101 Firefox/31.9");
// Citi requires native mode. Or it blocks.. "too old firefox"
pref("general.useragent.override.citi.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:2.0) Goanna/20160101 PaleMoon/26.0");
// Yuku fora don't like the Goanna slice (result: broken mobile site)
pref("general.useragent.override.yuku.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.9) Gecko/20100101 Firefox/38.9 PaleMoon/26.0");

// UA-Sniffing domains below have indicated no interest in supporting Pale Moon (BOO!)
pref("general.useragent.override.humblebundle.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:33.0) Gecko/20100101 Firefox/33.0 (Pale Moon)");
pref("general.useragent.override.privat24.ua","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:28.0) Gecko/20100101 Firefox/28.0");
pref("general.useragent.override.icloud.com","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:38.0) Gecko/20100101 Firefox/38.0 (Pale Moon)");

// UA-sniffing domains that are "app/vendor-specific" and don't like Pale Moon
pref("general.useragent.override.web.whatsapp.com","Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/37.0.2049.0 Safari/537.36");

// The following domains don't like the Goanna slice
pref("general.useragent.override.hitbox.tv","Mozilla/5.0 (Windows NT 6.1; WOW64; rv:31.9) Gecko/20100101 Firefox/31.9");

// Enable Firefox compatibility mode globally?
pref("general.useragent.compatMode.firefox", false);

// ****************** UI config ******************

pref("browser.tabs.insertRelatedAfterCurrent", false); //use old method of tabbed browsing instead of "Chrome" style
pref("browser.download.useDownloadDir", false); //don't use default download location as standard. ASK.
pref("browser.search.context.loadInBackground", true); //don't swap focus to the context search tab.
pref("browser.ctrlTab.previews", true);
pref("browser.allTabs.previews", true);
pref("browser.urlbar.trimURLs", false); //stop being a derp, Mozilla!
pref("browser.identity.ssl_domain_display", 1); //show domain verified SSL (blue)
pref("browser.urlbar.autoFill", true);
pref("browser.urlbar.autoFill.typed", true);

//Set tabs NOT on top
pref("browser.tabs.onTop",false); 

// ****************** Misc. config ******************

// Download manager
pref("browser.download.manager.flashCount", 10);
pref("browser.download.manager.scanWhenDone", false); //NIB, make sure to disable to prevent hangups
pref("browser.altClickSave", true); //SBaD,M! (#2)

//plugin kill timeout
pref("dom.ipc.plugins.timeoutSecs", 20);

//give people a choice for add-on updates.
pref("extensions.update.autoUpdateDefault", false);

//Improve memory handling for js
pref("javascript.options.mem.gc_per_compartment", true);
pref("javascript.options.mem.high_water_mark", 64);
pref("javascript.options.mem.max", -1);
pref("javascript.options.gc_on_memory_pressure", true);
pref("javascript.options.mem.disable_explicit_compartment_gc", true);
//add IGC and adjust time slice
pref("javascript.options.mem.gc_incremental",true);
pref("javascript.options.mem.gc_incremental_slice_ms",20);

//DOM
pref("dom.disable_window_status_change", false); //Allow status feedback by default
//Set max script runtimes to sane values
pref("dom.max_chrome_script_run_time", 90); //Some addons need ample time!
pref("dom.max_script_run_time", 20); //Should be plenty for a page script to do what it needs

//Image decoding tweaks
pref("image.mem.max_ms_before_yield", 20);

//store sessions less frequently to prevent redundant mem usage by storing too much
pref("browser.sessionstore.interval",60000); //every minute instead of every 10 seconds
pref("browser.sessionstore.privacy_level",1); //don't store session data (forms, etc) for secure sites
