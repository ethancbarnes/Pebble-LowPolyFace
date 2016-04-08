Pebble.addEventListener('showConfiguration', function() {
  var url = 'http://ethancbarnes.github.io/lowpolyconfig.html';
  console.log('Open config page: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  // Decode the user's preferences
  var configData = JSON.parse(decodeURIComponent(e.response));

  // Send to the watchapp via AppMessage
  var dict = {
    'AppKeyBackgroundColor': configData.background_color,
    'AppKeyVibrate': configData.vibrate,
    'AppKeyLeadingZero': configData.leadingzero,
    'AppKeyShowBattery': configData.batterymeter
  };

  // Send to the watchapp
  Pebble.sendAppMessage(dict, function() {
    console.log('Config data sent successfully!');
  }, function(e) {
    console.log('Error sending config data!');
  });
});
