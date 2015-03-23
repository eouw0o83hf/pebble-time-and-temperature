// Called when the watchface is opened
Pebble.addEventListener('ready',
	function(e) {
		console.log("Getting weather");
		getWeather();
	});

// Called when an AppMessage is received
Pebble.addEventListener('appmessage',
	function(e) {
		console.log("AppMessage received!");	
		getWeather();
	});

var xhrRequest = function(url, type, callback) {
	var xhr = new XMLHttpRequest();
	xhr.onload = function() {
		callback(this.responseText);
	};
	xhr.open(type, url);
	xhr.send();
};

// Request weather for given location
function locationSuccess(pos) {
	var url = "http://api.openweathermap.org/data/2.5/weather?lat=" +
      pos.coords.latitude + "&lon=" + pos.coords.longitude;
	
	xhrRequest(url, 'GET',
		function(responseText) {
			// the payload is JSON
			var json = JSON.parse(responseText);
			// convert from K
			var temperature = Math.round(((json.main.temp - 273.15) * 9 / 5) + 32);
			var conditions = json.weather[0].main;
			
			console.log("Temp is " + temperature + "; conditions are " + conditions);
			
			var dictionary = {
				0: temperature,
				1: conditions,
				// toString() these because the Pebble C library doesn't understand
				// how to parse floating-point numbers, so it doesn't recognize the
				// floating-point JSON value. Just make it a string.
				2: pos.coords.latitude.toString(),
				3: pos.coords.longitude.toString()
			};
			
			console.log("Sending to pebble: " + JSON.stringify(dictionary));
			
			Pebble.sendAppMessage(dictionary,
				function(e) {
					console.log("pebble send success");
				},
				function(e) {
					console.log("pebble send fail: " + e.error.message);
				});
		});
}

function locationError(err) {
	console.log("Error requesting location");
}

function getWeather() {
	navigator.geolocation.getCurrentPosition(
		locationSuccess,
		locationError,
		{timeout: 15000, maximumAge: 60000}	);
}