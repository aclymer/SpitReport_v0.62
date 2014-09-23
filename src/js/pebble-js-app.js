var spot_id;
var d = new Date();
var hr = d.getHours();
console.log(d,hr);
var direction;
var nearby_spot_id;
var county;
var water_temp;

function fetchNearby(pos) {
	var response;
	var coordinates = pos.coords;
	var req = new XMLHttpRequest();
	req.open('GET', "http://api.spitcast.com/api/spot/nearby?longitude="+coordinates.longitude+"&latitude="+coordinates.latitude, true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
//        		console.log(req.responseText);
				response = JSON.parse(req.responseText);
				if (response && response.length > 0) {
					var surfResult = response;
					nearby_spot_id = surfResult[0].spot_id;
					spot_id = nearby_spot_id;
					console.log("Nearest Spot Id: " + spot_id);
				}
				
			} 
			else {
				console.log("fetchNearby:HTTP " + req.status + " error");
				fetchNearby(pos);
			}
		}
	}
	req.send(null);
	req.onloadend = function() {
		return;
	};
}

function fetchCounty(Id) {
	var response;
	var req = new XMLHttpRequest();
	req.open('GET', "http://api.spitcast.com/api/spot/all", true);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if (req.status == 200) {
				response = JSON.parse(req.responseText);
				for (var i = 0; i < response.length; i++) {
					if (response[i].spot_id == Id) {
						county = response[i].county_name;
						var county_name = [];
						for (var j = 0; j < county.length; j++) {
							if (county[j] == " ") {county_name.push("-");}
							else {county_name.push(county[j]);}
						}
						county = county_name.join("");
						county = county.toLowerCase();
						console.log("County: " + county);
						fetchWater(county);
					}
				}
				
			}
		}
	}
	req.send(null);
	req.onloadend = function() {};
}

function fetchNeighbor(Id, dir) {
	var response;
	var req = new XMLHttpRequest();
	req.open('GET', "http://api.spitcast.com/api/spot/neighbors/" + Id + "/?direction=" + dir);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if (req.status == 200) {
//				console.log(req.responseText);
				response = JSON.parse(req.responseText);
				if (response && response.length > 0) {
					var len = response.length;
					if (dir == "above") {spot_id = response[len - 1].spot_id;}
					if (dir == "below") {spot_id = response[0].spot_id;}
					if (dir == "return") {spot_id = nearby_spot_id;}					
					console.log("Spot Id: " + spot_id);
				}
			}
			else {
				console.log("fetchNeighbor:HTTP " + req.status + " error");
			}
			
		}
	}
	req.send(null);
	req.onloadend = function() {
		fetchCounty(spot_id);
	};
}

function fetchWater(county_name) {
	var response;
	console.log("fetchWater: " + county_name);
	var req = new XMLHttpRequest();
	req.open('GET', "http://api.spitcast.com/api/county/water-temperature/" + county_name);
	req.onload = function(e) {
		if (req.readyState == 4) {
			if (req.status == 200) {
//				console.log(req.responseText);
				response = JSON.parse("[" + req.responseText + "]");
				if (response && response.length > 0) {
					water_temp = response[0].fahrenheit;
					console.log("Water Temp: " + water_temp)
				}
			}
			else {
				console.log("fetchWater:HTTP " + req.status + " error");
			}
		}
	}
	req.send(null);
	req.onloadend = function() {
		fetchSurf(spot_id);
	};
}

function fetchSurf(Id) {
	var response;
	console.log("fetchSurf:Id " + Id);
	var req = new XMLHttpRequest();
	req.open('GET', "http://api.spitcast.com/api/spot/forecast/" + Id + "/"); 
	req.onload = function(e) {
		if (req.readyState == 4) {
			if(req.status == 200) {
//        		console.log(req.responseText);
				response = JSON.parse(req.responseText);
				var shape, spot_name, icon_id, wind, surf_ht;
				if (response && response.length > 0) {
					var surfResult = response;
					surf_ht = surfResult[hr].size;
					shape = surfResult[hr].shape;
					wind = surfResult[hr].shape_detail.wind;
					spot_name = surfResult[hr].spot_name;
					if (shape == "p") { icon_id = 0;}
					else if (shape == "pf") { icon_id = 1;}
						else if (shape == "f") { icon_id = 2;}
							else if (shape == "fg") { icon_id = 3;}
								else if (shape == "g") { icon_id = 4;}
									else { icon_id = null;}
					console.log("Wind: " + wind);
					console.log("Shape: " + shape);
					console.log("Icon ID: " + icon_id);
					console.log("Spot Name: " + spot_name);
					console.log("Ht: " + surf_ht + " ft");
					Pebble.sendAppMessage({
						"icon_id": icon_id,
						"wind": wind,
						"spot_name": spot_name,
						"surf_ht": surf_ht + " ft",
						"water_temp": water_temp + "\u00B0F",
					});
				}
			} 
			else {
				console.log("fetchSurf:HTTP " + req.status + " error!");
			}
		}
	}
	req.send(null);
	req.onloadend = function () {};
}

function locationSuccess(pos) {
	console.warn("Location Found!");
	var coordinates = pos.coords;
	fetchNeighbor(spot_id,"refresh");//(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
	console.warn("location error (" + err.code + "): " + err.message);
	//	spot_id += 1;
	if (err.code == 3) {
	}
	else if (err.code == 1) {
		Pebble.sendAppMessage({
			"spot_name":"DENIED",
			"wind":"Location Svc."
		});
	}
		else {
			Pebble.sendAppMessage({
				"spot_name":"Loc Unavailable",
				"wind": NULL
			});
		}
}

var locationOptions = {"timeout": 30000, "maximumAge": 60000 }; 


Pebble.addEventListener("ready",
						function(e) {
							console.log("connect!" + e.ready);
							locationWatcher = window.navigator.geolocation.watchPosition(fetchNearby, locationError, locationOptions);
							console.log(e.type);
						});

Pebble.addEventListener("appmessage",
						function(e) {
							console.log(e.type);
							console.log("Index: " + e.payload.index);
							console.log("Message: " + e.payload.index);
							if (e.payload.index == "refresh") {
								window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
							}
							else if (e.payload.index == "return") {
								fetchNeighbor(nearby_spot_id, e.payload.index);
							}
							else {
								fetchNeighbor(spot_id, e.payload.index);
							}
						});

Pebble.addEventListener("webviewclosed",
						function(e) {
							console.log("webview closed");
							console.log(e.type);
							console.log(e.response);
						});
