
Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://csiebler.github.io/pebble-config-pages/simplr2-config.html';
  console.log('Showing configuration page: ' + url);

  Pebble.openURL(url);
});


Pebble.addEventListener('webviewclosed', function(e) {
  var configData = JSON.parse(decodeURIComponent(e.response));
  console.log('Configuration page returned: ' + JSON.stringify(configData));

  var dict = {};
  dict['KEY_HOURS_24'] = configData['hours_24'] ? 1 : 0;  // Send a boolean as an integer

  // Send to watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Send successful: ' + JSON.stringify(dict));
  }, function() {
    console.log('Send failed!');
  });
});
