/* Phone-side half of the colour / black-and-white setting.
 *
 * No Clay. Clay is the usual way to build a Pebble settings page, but it is an
 * npm dependency, and nothing in this repo's packaging installs one: the Nix
 * build runs `pebble build` over the checkout and no more. Vendoring its dist
 * to configure a single boolean is a poor trade, so the page below is written
 * out by hand — it is the same trick Clay uses underneath, a `data:` URI, so it
 * needs no hosting and works with the phone offline.
 *
 * The value is mirrored into localStorage only so the page can open with the
 * right option already selected. The watch is where it actually lives (see
 * src/c/modules/settings.c): the phone cannot read the watch's persistent
 * storage, and a watchface is loaded far more often than it is configured.
 */

var STORE_KEY = 'bwMode';

/* Where the page navigates to hand its answer back, as a snippet the page
 * carries with it. The phone app closes on `pebblejs://close#<response>`, but
 * it is not the only host: `pebble emu-app-config` writes the page to a local
 * file and passes an HTTP `return_to` in the query instead, and a page that
 * ignores it can never be round-tripped in the emulator. Reading the parameter
 * with the scheme as the fallback covers both.
 */
var BACK_FN =
  'function back(){' +
  'var m=location.search.match(/[?&]return_to=([^&]*)/);' +
  'return m?decodeURIComponent(m[1]):"pebblejs://close#";}';

/* Once a showConfiguration handler exists, the phone app shows the settings
 * gear for EVERY watch this face is installed on. Only emery ships both sets of
 * assets, so the others get told so rather than a toggle that does nothing.
 */
function isEmery() {
  try {
    return Pebble.getActiveWatchInfo().platform === 'emery';
  } catch (e) {
    /* getActiveWatchInfo throws on firmware older than 3.2. Nothing that old
     * runs emery, so treat the failure as "not emery".
     */
    return false;
  }
}

/* The face's own palette: navy sky, stone, and the gold of the frieze. */
var STYLE = [
  'body{margin:0;padding:28px 22px;background:#000055;color:#fff;',
  'font:16px/1.45 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}',
  'h1{margin:0 0 4px;font-size:19px;letter-spacing:.14em;text-transform:uppercase;color:#ffaa00}',
  'p{margin:0 0 22px;color:#aaaaaa;font-size:14px}',
  'label{display:flex;align-items:center;gap:12px;padding:16px;margin-bottom:10px;',
  'background:#ffffff;color:#000000;border-radius:10px}',
  'label input{width:20px;height:20px;margin:0;accent-color:#000055}',
  'button{width:100%;padding:15px;margin-top:18px;border:0;border-radius:10px;',
  'font-size:17px;font-weight:600;background:#ffaa00;color:#000000}',
  'button.secondary{background:transparent;color:#aaaaaa;font-weight:400;margin-top:6px}'
].join('');

function head() {
  return [
    '<!DOCTYPE html><html><head><meta charset="utf-8">',
    '<meta name="viewport" content="width=device-width,initial-scale=1">',
    '<title>Lemming Brothers</title><style>', STYLE, '</style></head><body>',
    '<h1>Lemming Brothers</h1>'
  ].join('');
}

function settingsPage(bw) {
  return [
    head(),
    '<p>Emery has a colour screen, so the facade can be shown either way.</p>',
    '<label><input type="radio" name="m" value="0"', bw ? '' : ' checked',
    '><span>Colour</span></label>',
    '<label><input type="radio" name="m" value="1"', bw ? ' checked' : '',
    '><span>Black and white</span></label>',
    '<button onclick="save()">Save</button>',
    '<button class="secondary" onclick="location.href=back()">Cancel</button>',
    '<script>', BACK_FN,
    'function save(){',
    'var v=document.querySelector("input[name=m]:checked").value;',
    /* The response comes back through webviewclosed as one encoded string, so
     * the page has to encode it here. Cancel returns with nothing appended,
     * which is what the empty-response check on the other side looks for.
     */
    'location.href=back()+encodeURIComponent(JSON.stringify({BW_MODE:Number(v)}));',
    '}<\/script></body></html>'
  ].join('');
}

function noSettingsPage() {
  return [
    head(),
    '<p>This watch has nothing to configure. The colour / black-and-white ',
    'choice is an Emery setting: the other supported watches either have no ',
    'colour screen to give up, or no black-and-white artwork to switch to.</p>',
    '<button onclick="location.href=back()">Close</button>',
    '<script>', BACK_FN, '<\/script></body></html>'
  ].join('');
}

Pebble.addEventListener('showConfiguration', function () {
  var bw = localStorage.getItem(STORE_KEY) === '1';
  var html = isEmery() ? settingsPage(bw) : noSettingsPage();
  Pebble.openURL('data:text/html,' + encodeURIComponent(html));
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) {
    /* Cancelled, or closed with the system back gesture. */
    return;
  }

  var config;
  try {
    config = JSON.parse(decodeURIComponent(e.response));
  } catch (err) {
    console.log('lemming: unreadable config response: ' + e.response);
    return;
  }
  if (typeof config.BW_MODE === 'undefined') {
    return;
  }

  var bw = config.BW_MODE ? 1 : 0;
  localStorage.setItem(STORE_KEY, String(bw));
  Pebble.sendAppMessage({ BW_MODE: bw }, function () {
    console.log('lemming: mode set to ' + (bw ? 'black and white' : 'colour'));
  }, function (err) {
    /* The watch keeps whatever it had; the next save tries again. Nothing is
     * retried here, because a watchface out of range is the normal case and a
     * queued message would arrive at an unpredictable moment.
     */
    console.log('lemming: could not send mode: ' + JSON.stringify(err));
  });
});
