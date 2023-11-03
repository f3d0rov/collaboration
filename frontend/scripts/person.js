
function getElemBottomCoordsForConnector (elem) {
	let br = elem.getBoundingClientRect ();
	return {
		x: (br.left + br.right) / 2 + window.scrollX,
		y: br.bottom - 16 + window.scrollY
	};
}

function getElemTopCoordsForConnector (elem) {
	let br = elem.getBoundingClientRect ();
	return {
		x: (br.left + br.right) / 2 + window.scrollX,
		y: br.top + 16 + window.scrollY
	};
}

function genConnector (topPoint, bottomPoint) {
	let template = document.getElementById ("timelineConnectorTemplate");
	let clone = template.cloneNode (true);
	line = clone.querySelector ('line');

	clone.classList.remove ('template');
	clone.classList.add ('actualConnector');
	clone.style.left = (topPoint.x - 4) + 'px';
	clone.style.top = topPoint.y + 'px';
	clone.setAttribute ('height', (bottomPoint.y - topPoint.y) + 'px');
	line.setAttribute('y2', bottomPoint.y - topPoint.y);
	template.parentElement.appendChild (clone);

	return clone;
}

function connect (i, a, b) {
	let aIcon = a.querySelector('.timelineElemIcon');
	let bIcon = b.querySelector('.timelineElemIcon');

	let topPoint = getElemBottomCoordsForConnector (aIcon);
	let bottomPoint = getElemTopCoordsForConnector (bIcon);

	let clone = genConnector (topPoint, bottomPoint);
	clone.id = "connector_" + i;
}

function generateConnectors (ev) {
	let nodes = document.querySelectorAll ('.timelineElem');
	let elems = [];

	for (let i of nodes) { // Not connecting templates
		if (!i.classList.contains ('template')) {
			elems.push (i);
		}
	}

	for (let i = 0; i < elems.length - 1; i++) {
		connect (i, elems[i], elems[i + 1]);
	}
}

function reconnect (ev) {
	let connectors = document.querySelectorAll ('.actualConnector');
	generateConnectors ();
	for (let i of connectors) {
		i.remove();
	}
}

function setByClass (elem, classname, innerHtml) {
	let sub = elem.querySelector ('.' + classname);
	sub.innerHTML = innerHtml;
}

function generateLookupUrl (lookupStr) {
	let keys = lookupStr.split (' ');
	if (keys.length == 0) return "https://developer.mozilla.org/en-US/docs/Web/JavaScript";
	let res = "https://www.google.com/search?q=" + keys[0];
	for (let i = 1; i < keys.length; i++) res += "+" + keys[i];
	return res;
}

function editEvent (id) {
	console.log ("edit event #" + id);
}

function reportEvent (id) {
	console.log ("report event #" + id);
}

function generateTimepoint (eventId, icon, titleHtml, date, body = "", lookupStr = "") {
	let template = document.getElementById ('timelineElemTemplate');
	let clone = template.cloneNode (true);
	clone.classList.remove ('template');
	setByClass (clone, 'timepointTitle', titleHtml);
	setByClass (clone, 'timepoint', date);
	setByClass (clone, 'timepointBody', body);
	clone.id = "timepoint_" + eventId;
	clone.querySelector ('.timelineElemIcon').setAttribute ('src', icon);

	if (lookupStr == "") {
		clone.querySelector("#lookupEntry").classList.add ("hidden");	
	} else {
		let url = generateLookupUrl (lookupStr);
		clone.querySelector ('#lookupEntry').addEventListener (
			'click',
			() => {
				window.open (url,'_blank', 'noopener');
			}
		);
	}

	clone.querySelector ('#editEntry').addEventListener (
		'click',
		() => { if (demandAuth()) { editEvent(eventId) } }
	);

	clone.querySelector ('#reportEntry').addEventListener (
		'click',
		() => { if (demandAuth()) { reportEvent(eventId) } }	
	);

	template.parentElement.insertBefore (clone, template);

	return clone;
}

function linkToNameUrlObj (obj) { 
	return '<a href="' + obj.url + '">' + obj.name + '</a>';
}

function generateFoundationTimepoint (event) {
	generateTimepoint (
		event.id,
		'/resources/timeline/band.svg',
		'Основание группы ' + linkToNameUrlObj (event.band),
		event.date,
		'body' in event ? event.body : "",
		"Listen to " + event.band.name
	);
}

function generateSingleTimepoint (event) {
	generateTimepoint (
		event.id,
		'/resources/timeline/song.svg',
		'Сингл ' + linkToNameUrlObj (event.band) + ' - ' + event.song,
		event.date,
		'body' in event ? event.body : "",
		"Listen to " + event.band.name + " - " + event.song
	);
}

function generateAlbumTimepoint (event) {
	generateTimepoint (
		event.id,
		'/resources/timeline/album.svg', 
		'Альбом ' + linkToNameUrlObj (event.band) + ' - ' + linkToNameUrlObj (event.album),
		event.date,
		'body' in event ? event.body : "",
		"Listen to " + event.band.name + " - " + event.album.name
	);
}

function defaultTimepointGenerator (event) {
	// TODO: report error to server
}

function generateEventList (events) {
	const timelineElemGenerators = {
		'band_foundation': generateFoundationTimepoint,
		'single': generateSingleTimepoint,
		'album': generateAlbumTimepoint
	};
	for (let i of events) {
		console.log (i);
		if (i.type in timelineElemGenerators) timelineElemGenerators [i.type](i);
		else defaultTimepointGenerator (i);
	}
	generateConnectors ();
}

var testData = [
	{
		id: 20,
		type: 'band_foundation',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		date: '1988',
		collaborators: [],
		body: "В 1994 году Резнор сказал в интервью, что выбрал для группы название «Nine Inch Nails», потому что оно хорошо звучало и прошло проверку временем (большинство названий спустя какое-то время начинали звучать плохо), а также легко превращалось в аббревиатуру."
	}, 

	{
		id: 21,
		type: 'single',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		song: "Down In It",
		date: '15.09.1988',
		collaborators: []
	},

	{
		id: 22,
		type: 'album',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		album: {
			name: 'Pretty Hate Machine',
			url: '/a?=12'
		},

		date: '20.10.1988',
		collaborators: []
	},
	
	{
		id: 23,
		type: 'single',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		song: "Head Like A Hole",
		date: '22.03.1989',
		collaborators: []
	},
	{
		id: 36,
		type: 'single',
		band: { 
			name: 'Nine Inch Nails',
			url: '/b?=12'
		},
		song: 'Sin',
		date: '10.10.1990',
		collaborators: []
	}
];

function generate404page () {
	console.log (404);
}

async function pullPersonData () {
	const params = new URLSearchParams (window.location.search);
	let id = params.get ("id");
	let apiEndpoint = "/api/p";
	let apiReqBody = {
		"id": parseInt(id)
	};
	
	let req = await fetch (
		apiEndpoint,
		{
			"method": "POST",
			"body": JSON.stringify(apiReqBody)
		}
	);

	if (req.status == 404) {
		generate404page();
	} else {
		let resp = await req.json ();
		document.getElementById ("personName").innerHTML = resp.name;
		document.getElementById ("personLifetime").innerHTML = 
			"(" + dateToString(resp.start_date) + ('end_date' in resp ? " - " + dateToString(resp.end_date) : "") + ")";
		document.getElementById ("personBio").innerHTML = resp.description;
		if ("picture_path" in resp) document.getElementById ("personImage").setAttribute ("src", resp.picture_path);
	}
}

function dateToString (date) {
	abc = date.split ("-");
	return abc[2] + "." + abc[1] + "." + abc[0];
}

function generatePage (ev) {
	pullPersonData ();
	// TODO: pull event data from API
	generateEventList (testData);

}

window.addEventListener ('load', generatePage);
window.addEventListener ('resize', reconnect);